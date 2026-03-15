#include "apply.hpp"
#include <gtest/gtest.h>

TEST(ApplyFunction, EmptyVector) {
    std::vector<int> data{};
    ApplyFunction<int>(data, [](int& value) {
        value += 1;
    });
    EXPECT_TRUE(data.empty());
}

TEST(ApplyFunction, NegativeThreadCount) {
    std::vector<int> data{1, 2, 3, 4, 5};
    ApplyFunction<int>(data, [](int& value) {
        value *= 2;
    }, -10);
    const std::vector<int> expected_data{2, 4, 6, 8, 10};
    EXPECT_EQ(data, expected_data);
}

TEST(ApplyFunction, ZeroThreadCount) {
    std::vector<int> data{1, 2, 3, 4, 5};
    ApplyFunction<int>(data, [](int& value) {
        value += 3;
    }, 0);
    const std::vector<int> expected_data{4, 5, 6, 7, 8};
    EXPECT_EQ(data, expected_data);
}

TEST(ApplyFunction, SingleThread) {
    std::vector<int> data(10);
    std::iota(data.begin(), data.end(), 0);
    ApplyFunction<int>(data, [](int& value) {
        value += 7;
    }, 1);
    const std::vector<int> expected_data{7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    EXPECT_EQ(data, expected_data);
}

TEST(ApplyFunction, MultiThread) {
    std::vector<int> data(10);
    std::iota(data.begin(), data.end(), 0);
    ApplyFunction<int>(data, [](int& value) {
        value += 7;
    }, 3);
    const std::vector<int> expected_data{7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    EXPECT_EQ(data, expected_data);
}

TEST(ApplyFunction, DataSizeLessThanThreadCount) {
    std::vector<int> data{2, 4, 6, 8, 10};
    ApplyFunction<int>(data, [](int& value) {
        value /= 2;
    }, 25);
    const std::vector<int> expected_data{1, 2, 3, 4, 5};
    EXPECT_EQ(data, expected_data);
}

TEST(ApplyFunction, SingleThreadAndMultiThread) {
    std::vector<int> singleThreadData(7777);
    std::vector<int> multiThreadData(7777);
    std::iota(singleThreadData.begin(), singleThreadData.end(), 0);
    multiThreadData = singleThreadData;
    auto transform = [](int& value) {
        value *= 2;
        value += 7;
        value /= 3;
    };
    ApplyFunction<int>(singleThreadData, transform, 1);
    ApplyFunction<int>(multiThreadData, transform, 7);
    EXPECT_EQ(singleThreadData, multiThreadData);
}