#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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

class thread_pool {
    std::mutex mtx;
    std::condition_variable cond_var;
    size_t max_threads{0};
    bool stop{false};
    std::queue<std::function<void()>> tasks;
    std::vector<std::jthread> threads;
public:
    explicit thread_pool(size_t n) {
        if (n > 0) {
            max_threads = n;
        } else {
            max_threads = 1;
        }
        threads.reserve(max_threads);
        for (int i = 0; i < max_threads; ++i) {
            threads.emplace_back([&] {
                while (true) {
                    std::unique_lock<std::mutex> lock(mtx);
                    cond_var.wait(lock, [&] {
                        return (stop || (tasks.size() > 0));
                    });
                    if (stop && tasks.empty()) {
                        return;
                    }
                    auto task = std::move(tasks.front());
                    tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    ~thread_pool() {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
        cond_var.notify_all();
    }

    template <typename T>
    my_future<T> submit(std::function<T()> func) {
        auto state = std::make_shared<shared_state<T>>();
        std::lock_guard<std::mutex> lock(mtx);
        tasks.emplace([state, func]() {
            T result = func();
            std::lock_guard<std::mutex> state_lock(state -> mutex);
            state -> value = std::move(result);
            state -> ready = true;
            state -> cond_var.notify_all();
        });
        cond_var.notify_one();
        return my_future<T>(state);
    }
};