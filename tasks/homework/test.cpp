#include "mutex.hpp"
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

TEST(mutex, try_lock) {
    mutex mtx;
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
    EXPECT_TRUE(mtx.try_lock());
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::LOCK_NO_WAIT);
    mtx.unlock();
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
}

TEST(mutex, try_lock_fail) {
    mutex mtx;
    mtx.lock();
    EXPECT_FALSE(mtx.try_lock());
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::LOCK_NO_WAIT);
    mtx.unlock();
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
}

TEST(mutex, lock_and_unlock_repeat) {
    mutex mtx;
    mtx.lock();
    mtx.unlock();
    mtx.lock();
    mtx.unlock();
    mtx.lock();
    mtx.unlock();
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
}

TEST(mutex, lock_wait_state) {
    mutex mtx;
    std::atomic<bool> atomic_past_perfect_continuous{};
    std::atomic<bool> atomic_present_perfect_continuous{};
    std::atomic<bool> atomic_future_perfect_continuous{};
    mtx.lock();
    std::jthread other_thread([&]() {
        atomic_past_perfect_continuous.store(true);
        mtx.lock();
        atomic_present_perfect_continuous.store(true);
        mtx.unlock();
        atomic_future_perfect_continuous.store(true);
    });
    while (!atomic_past_perfect_continuous.load()) {
        std::this_thread::yield();
    }
    while (mtx.native_handle() -> load() != lock_state::LOCK_WAIT) {
        std::this_thread::yield();
    }
    EXPECT_FALSE(atomic_present_perfect_continuous.load());
    mtx.unlock();
    while (!atomic_future_perfect_continuous.load()) {
        std::this_thread::yield();
    }
    EXPECT_TRUE(atomic_present_perfect_continuous.load());
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
}

TEST(mutex, many_threads_one_result) {
    const int count = 7;
    const int n = 7777;
    mutex mtx;
    int result{};
    std::vector<std::thread> threads;
    threads.reserve(count);
    for (int i = 0; i < count; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < n; ++j) {
                mtx.lock();
                ++result;
                mtx.unlock();
            }
        });
    }
    for (int i = 0; i < count; ++i) {
        threads[i].join();
    }
    EXPECT_EQ(mtx.native_handle() -> load(), lock_state::UNLOCK);
    EXPECT_EQ(result, count * n);
}