#pragma once

#include <optional>
#include <vector>
#include <stdexcept>
#include <condition_variable>  

template <class T>
class BufferedChannel {
    int m_capacity{};
    std::mutex m_mtx;
    std::condition_variable m_send;
    std::condition_variable m_recv;
    bool m_closed{};
    std::vector<T> m_buffer;
    int m_size{};
    int m_begin{};
    int m_end{};
public:
    explicit BufferedChannel(int size) : m_capacity(size), m_buffer(size) {
        // Your code goes here
    }

    void Send(const T& value) {
        // Your code goes here
        std::unique_lock lock{m_mtx};
        m_send.wait(lock, [this]() { return ((m_size < m_capacity) || m_closed); });
        if (m_closed) {
            throw std::runtime_error("The channel is closed!");
        }
        m_buffer[m_end] = value;
        ++m_size;
        ++m_end;
        if (m_end == m_capacity) {
            m_end = 0;
        }
        lock.unlock();
        m_recv.notify_one();
    }

    std::optional<T> Recv() {
        // Your code goes here
        std::unique_lock lock{m_mtx};
        m_recv.wait(lock, [this]() { return ((m_size > 0) || m_closed); });
        if (m_size == 0) {
            return std::nullopt;
        }
        T value = std::move(m_buffer[m_begin]);
        --m_size;
        ++m_begin;
        if (m_begin == m_capacity) {
            m_begin = 0;
        }
        lock.unlock();
        m_send.notify_one();
        return value;
    }

    void Close() {
        // Your code goes here
        std::lock_guard guard{m_mtx};
        m_closed = true;
        m_send.notify_all();
        m_recv.notify_all();
    }
};