#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct queue_info {
    size_t protocol_version;
    size_t ring_buffer_size;
    std::atomic<size_t> consumer_read;
    std::atomic<size_t> producer_write;
};

struct message_header {
    char type[32];
    std::atomic<size_t> size;
};

class my_queue {
    int shm_fd;
    void * shm_ptr;
    size_t shm_size;
    queue_info * info_ptr;
    std::byte * ring_buffer_ptr;
public:
    my_queue(size_t size, bool create) {
        shm_ptr = nullptr;
        info_ptr = nullptr;
        ring_buffer_ptr = nullptr;
        shm_fd = -1;
        const size_t ring_offset = ((sizeof(queue_info) + alignof(message_header) - 1) / 
                                     alignof(message_header)) * alignof(message_header);
        shm_size = ring_offset + size;
        int flags = O_RDWR;
        if (create) {
            flags |= O_CREAT;
        }
        const size_t expected_protocol_version = 17042005;
        if (!create) {
            while (true) {
                shm_fd = shm_open("/mpsc", flags, 0777);
                if (shm_fd == -1) {
                    continue;
                }
                shm_ptr = mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shm_ptr == MAP_FAILED) {
                    shm_ptr = nullptr;
                    close(shm_fd);
                    shm_fd = -1;
                    continue;
                }
                info_ptr = reinterpret_cast<queue_info *>(shm_ptr);
                if (info_ptr -> protocol_version != expected_protocol_version) {
                    munmap(shm_ptr, shm_size);
                    shm_ptr = nullptr;
                    close(shm_fd);
                    shm_fd = -1;
                    continue;
                }
                ring_buffer_ptr = reinterpret_cast<std::byte *>(shm_ptr) + ring_offset;
                return;
            }
        }
        shm_fd = shm_open("/mpsc", flags, 0777);
        if (shm_fd == -1) {
            return;
        }
        if (ftruncate(shm_fd, shm_size) == -1) {
            close(shm_fd);
            shm_fd = -1;
            return;
        }
        shm_ptr = mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED) {
            shm_ptr = nullptr;
            close(shm_fd);
            shm_fd = -1;
            return;
        }
        info_ptr = reinterpret_cast<queue_info *>(shm_ptr);
        ring_buffer_ptr = reinterpret_cast<std::byte *>(shm_ptr) + ring_offset;
        info_ptr -> protocol_version = expected_protocol_version;
        info_ptr -> ring_buffer_size = size;
        info_ptr -> consumer_read.store(0);
        info_ptr -> producer_write.store(0);
        std::memset(ring_buffer_ptr, 0, size);
    }

    ~my_queue() {
        if (shm_ptr != nullptr) {
            munmap(shm_ptr, shm_size);
        }
        if (shm_fd != -1) {
            close(shm_fd);
        }
    }

    queue_info * info() {
        return info_ptr;
    }

    std::byte * ring_buffer() {
        return ring_buffer_ptr;
    }
};

class producer_node {
    my_queue queue;
public:
    producer_node(size_t size) : queue(size, true) {
    }

    bool send(const std::string& type, const void * data, size_t size) {
        queue_info * info = queue.info();
        if (info == nullptr) {
            return false;
        }
        if (type.size() >= 32) {
            return false;
        }
        const size_t alignment = alignof(message_header);
        const size_t full_size = ((sizeof(message_header) + size + alignment - 1) / alignment) * alignment;
        size_t buffer_size = info -> ring_buffer_size;
        if (full_size > buffer_size) {
            return false;
        }
        while (true) {
            size_t read_pos = info -> consumer_read.load();
            size_t write_pos = info -> producer_write.load();
            if (write_pos < read_pos) {
                info -> consumer_read.store(write_pos);
                continue;
            }
            size_t used = write_pos - read_pos;
            if (used >= buffer_size) {
                return false;
            }
            size_t free = buffer_size - used;
            if (free < full_size) {
                return false;
            }
            const size_t offset = write_pos % buffer_size;
            const size_t remain = buffer_size - offset;
            if (remain >= full_size) {
                const size_t new_offset = info -> producer_write.fetch_add(full_size) % buffer_size;
                if (new_offset + full_size > buffer_size) {
                    return false;
                }
                message_header * header = reinterpret_cast<message_header *>(queue.ring_buffer() + new_offset);
                header -> size.store(0);
                std::memset(header -> type, 0, 32);
                std::memcpy(header -> type, type.c_str(), type.size());
                std::byte * message_begin = reinterpret_cast<std::byte *>(header) + sizeof(message_header);
                std::memcpy(message_begin, data, size);
                header -> size.store(size + 1);
                return true;
            }
            if (remain < sizeof(message_header)) {
                continue;
            }
            if (free < remain + full_size) {
                return false;
            }
            message_header * header = reinterpret_cast<message_header *>(queue.ring_buffer() + offset);
            header -> size.store((buffer_size - offset) + 1);
            std::memset(header -> type, 0, 32);
            std::memcpy(header -> type, type.c_str(), type.size());
            info -> producer_write.fetch_add(remain);
        }
    }
};

class consumer_node {
    my_queue queue;
public:
    consumer_node(size_t size) : queue(size, false) {
    }

    bool receive(const std::string& wanted_type, std::vector<std::byte>& result_message) {
        queue_info * info = queue.info();
        if (info == nullptr) {
            return false;
        }
        size_t buffer_size = info -> ring_buffer_size;
        while (true) {
            size_t read_pos = info -> consumer_read.load();
            size_t write_pos = info -> producer_write.load();
            if (read_pos > write_pos) {
                info -> consumer_read.store(write_pos);
                continue;
            }
            if (read_pos == write_pos) {
                return false;
            }
            size_t offset = read_pos % buffer_size;
            message_header * header = reinterpret_cast<message_header *>(queue.ring_buffer() + offset);
            if (offset + sizeof(message_header) > buffer_size) {
                info -> consumer_read.store(read_pos + (buffer_size - offset));
                continue;
            }
            size_t result = header -> size.load();
            if (result == 0) {
                return false;
            }
            size_t message_size = result - 1;
            const size_t alignment = alignof(message_header);
            const size_t full_size = ((sizeof(message_header) + message_size + alignment - 1) / alignment) * alignment;
            if (offset + full_size > buffer_size) {
                info -> consumer_read.store(read_pos + (buffer_size - offset));
                continue;
            }
            if (wanted_type == std::string(header -> type)) {
                result_message.resize(message_size);
                std::byte * message_begin = reinterpret_cast<std::byte *>(header) + sizeof(message_header);
                std::memcpy(result_message.data(), message_begin, message_size);
                info -> consumer_read.store(read_pos + full_size);
                return true;
            }
            info -> consumer_read.store(read_pos + full_size);
        }
    }
};