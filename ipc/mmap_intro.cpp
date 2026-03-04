#include <utils/error.hpp>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <format>

sem_t *MakeSemaphore(const char *path)
{
    sem_t *sema = sem_open(path, O_CREAT, 0666, 0);

    if (sema == SEM_FAILED) {
        throw std::runtime_error{std::format("Opening semaphore failed, error: {}", strerror(errno))};
    }

    return sema;
}

struct SharedData
{
    static constexpr size_t DataSize = 4096;
    static constexpr size_t BufferSize = DataSize - sizeof(uint32_t);

    uint32_t m_messageSize{};
    char buffer[BufferSize]{};
};

int main() {
    static constexpr auto WriteDoneSema = "/write_sema";
    static constexpr auto ReadDoneSema = "/read_done_sema";

    sem_t* writeDoneSema = MakeSemaphore(WriteDoneSema);
    sem_t* readDoneSema = MakeSemaphore(ReadDoneSema);

    void *sharedMemoryPtr =
        mmap(nullptr, SharedData::DataSize, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (sharedMemoryPtr == MAP_FAILED) {
        throw std::runtime_error{std::format("mmap failed, error: {}", strerror(errno))};
    }
    
    SharedData *sharedData = new (sharedMemoryPtr) SharedData{};

    const pid_t pid = fork();
    CHECK_ERROR(pid);

    if (pid == 0) {
        while (true) {
            CHECK_ERROR(sem_wait(writeDoneSema));

            if (sharedData->m_messageSize == 0) {
                break;
            }

            std::cout << std::format("[CHILD]: read message of size: {}", sharedData->m_messageSize) << std::endl;
            std::cout << std::format("[CHILD]: content: {}", sharedData->buffer) << std::endl;

            CHECK_ERROR(sem_post(readDoneSema));
        }

        CHECK_ERROR(sem_close(readDoneSema));
        CHECK_ERROR(sem_close(writeDoneSema));

        sharedData->~SharedData();
    } else {
        std::string input;
        while (std::getline(std::cin, input)) {
            sharedData->m_messageSize = input.size();
            std::memset(sharedData->buffer, 0, SharedData::BufferSize);
            std::memcpy(sharedData->buffer, input.data(), std::min(input.size(), SharedData::BufferSize));

            std::cout << std::format("[PARENT]: wrote {} bytes", input.size()) << std::endl;

            CHECK_ERROR(sem_post(writeDoneSema));
            CHECK_ERROR(sem_wait(readDoneSema));
        }

        sharedData->m_messageSize = 0;
        CHECK_ERROR(sem_post(writeDoneSema));

        CHECK_ERROR(wait(nullptr));
        CHECK_ERROR(sem_unlink(ReadDoneSema));
        CHECK_ERROR(sem_unlink(WriteDoneSema));
        sharedData->~SharedData();
        CHECK_ERROR(munmap(sharedMemoryPtr, SharedData::DataSize));
    }

    std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT") << std::endl;

    return 0;
}