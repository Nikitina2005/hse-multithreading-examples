#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <numeric>

template <typename T>
void ApplyFunction(std::vector<T>& data, 
                   const std::function<void(T&)>& transform, 
                   const int threadCount = 1) {
    if (data.size() == 0) {
        return;
    }
    int realThreadCount{};
    if (threadCount < 1) {
        realThreadCount = 1;
    } else if (data.size() < threadCount) {
        realThreadCount = data.size();
    } else {
        realThreadCount = threadCount;
    }
    std::vector<std::jthread> jthreadPull;
    jthreadPull.reserve(realThreadCount);
    const auto elementCount = data.size() / realThreadCount;
    for (int i = 0; i < realThreadCount; ++i) {
        jthreadPull.emplace_back([&, i]() {
            const auto begin = i * elementCount;
            const auto end = (i == realThreadCount - 1) ? data.size()
                                                        : (i + 1) * elementCount;
            for (int j = begin; j < end; ++j) {
                transform(data[j]);
            }
        });
    }
}
