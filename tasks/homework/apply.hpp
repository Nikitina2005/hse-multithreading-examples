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
    for (int i = 0; i < realThreadCount; ++i) {
        jthreadPull.emplace_back([&, i]() {
            for (int j = i; j < data.size(); j += realThreadCount) {
                transform(data[j]);
            }
        });
    }
}