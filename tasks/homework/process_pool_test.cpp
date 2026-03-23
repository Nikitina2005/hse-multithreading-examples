#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>
#include <unistd.h>

#include "process_pool.hpp"

TEST(process_pool, submit_get) {
    process_pool pool(4);
    auto task_future = pool.submit<int>([] {
        return 18 + 12;
    });
    EXPECT_TRUE(task_future.valid());
    EXPECT_EQ(task_future.get(), 30);
    EXPECT_FALSE(task_future.valid());
}

TEST(process_pool, wait_then_get) {
    process_pool pool(4);
    auto task_future = pool.submit<int>([] {
        return 2 * 2;
    });
    task_future.wait();
    EXPECT_EQ(task_future.get(), 4);
}

TEST(process_pool, wait_for_ready) {
    process_pool pool(4);
    auto task_future = pool.submit<int>([] {
        return 1 << 20;
    });
    EXPECT_EQ(
        task_future.wait_for(std::chrono::seconds(3)),
        my_future<int>::ready);
    EXPECT_EQ(task_future.get(), 1048576);
}

TEST(process_pool, wait_until_ready) {
    process_pool pool(4);
    auto task_future = pool.submit<bool>([] {
        return true;
    });
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(7);
    EXPECT_EQ(task_future.wait_until(until), my_future<bool>::ready);
    EXPECT_EQ(task_future.get(), true);
}

TEST(process_pool, many_submits) {
    process_pool pool(2);
    EXPECT_EQ(pool.submit<int>([] {
        return 1 << 5;
    }).get(), 32);
    EXPECT_EQ(pool.submit<int>([] {
        return 1 << 10;
    }).get(), 1024);
    EXPECT_EQ(pool.submit<int>([] {
        return 1 << 20;
    }).get(), 1048576);
}

TEST(process_pool, zero) {
    process_pool pool(0);
    EXPECT_EQ(pool.submit<int>([] {
        return 42;
    }).get(), 42);
}

TEST(process_pool, other_many_submits) {
    constexpr int n = 8;
    process_pool pool(4);
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += pool.submit<int>([i] {
            return i * i;
        }).get();
    }
    const int expected = 0 + 1 + 4 + 9 + 16 + 25 + 36 + 49;
    EXPECT_EQ(sum, expected);
}