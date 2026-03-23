#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <sys/wait.h>
#include <unistd.h>

template <typename T>
struct shared_state {
    mutable std::mutex mutex;
    std::condition_variable cond_var;
    std::optional<T> value;
    bool ready{false};
};

template <typename T>
class my_future {
    std::shared_ptr<shared_state<T>> state;

public:
    enum my_future_status {
        ready,
        timeout,
        deferred
    };

    explicit my_future(std::shared_ptr<shared_state<T>> input_state) : state(std::move(input_state)) {
    }

    T get() {
        std::unique_lock<std::mutex> lock(state -> mutex);
        state -> cond_var.wait(lock, [&] {
            return state -> ready;
        });
        T result = std::move(*(state -> value));
        state.reset();
        return result;
    }

    void wait() const {
        std::unique_lock<std::mutex> lock(state -> mutex);
        state -> cond_var.wait(lock, [&] {
            return state -> ready;
        });
    }

    template <class rep, class period>
    my_future_status wait_for(const std::chrono::duration<rep, period>& rel_time) const {
        std::unique_lock<std::mutex> lock(state -> mutex);
        auto result = state -> cond_var.wait_for(lock, rel_time, [&] {
            return state -> ready;
        });
        if (result) {
            return my_future_status::ready;
        }
        return my_future_status::timeout;
    }

    template <class clock, class duration>
    my_future_status wait_until(const std::chrono::time_point<clock, duration>& abs_time) const {
        std::unique_lock<std::mutex> lock(state -> mutex);
        auto result = state -> cond_var.wait_until(lock, abs_time, [&] {
            return state -> ready;
        });
        if (result) {
            return my_future_status::ready;
        }
        return my_future_status::timeout;
    }

    bool valid() const noexcept {
        return static_cast<bool>(state);
    }
};

class process_pool {
    size_t max_children;
    std::mutex mtx;
    std::condition_variable cond_var;
    size_t running_children{0};
public:
    explicit process_pool(size_t n) {
        if (n == 0) {
            max_children = 1;
        } else {
            max_children = n;
        }
    }

    template <typename T>
    my_future<T> submit(std::function<T()> func) {
        auto state = std::make_shared<shared_state<T>>();
        std::unique_lock<std::mutex> lock(mtx);
        cond_var.wait(lock, [&] {
            return running_children < max_children;
        });
        ++running_children;
        lock.unlock();
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            lock.lock();
            if (running_children > 0) {
                --running_children;
            }
            cond_var.notify_all();
            return my_future<T>(state);
        }
        const pid_t pid = fork();
        if (pid < 0) {
            close(pipefd[0]);
            close(pipefd[1]);
            lock.lock();
            if (running_children > 0) {
                --running_children;
            }
            cond_var.notify_all();
            return my_future<T>(state);
        }
        if (pid == 0) {
            close(pipefd[0]);
            T child_result = func();
            write(pipefd[1], &child_result, sizeof(T));
            close(pipefd[1]);
            exit(0);
        }
        close(pipefd[1]);
        const int read_fd = pipefd[0];
        T result{};
        read(read_fd, &result, sizeof(T));
        std::lock_guard<std::mutex> value_lock(state -> mutex);
        state -> value = result;
        state -> ready = true;
        state -> cond_var.notify_all();
        close(read_fd);
        waitpid(pid, nullptr, 0);
        lock.lock();
        if (running_children > 0) {
            --running_children;
        }
        cond_var.notify_all();
        return my_future<T>(state);
    }
};
