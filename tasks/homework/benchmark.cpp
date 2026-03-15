#include "apply.hpp"
#include <benchmark/benchmark.h>

static void BM_SmallDataSize_EasyTransform_SingleThread(benchmark::State& state) {
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        auto values = data;
        ApplyFunction<int>(values, [](int& value) {
            value += 3;
        }, 1);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}

static void BM_SmallDataSize_EasyTransform_MultiThread(benchmark::State& state) {
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        auto values = data;
        ApplyFunction<int>(values, [](int& value) {
            value += 3;
        }, 10);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}

static void BM_BigDataSize_DifficultTransform_SingleThread(benchmark::State& state) {
    std::vector<double> data(1000000, 1.0);
    for (auto _ : state) {
        auto values = data;
        ApplyFunction<double>(data, [](double& value) {
            double temp_value = value;
            for (int i = 0; i < 1000; ++i) {
                temp_value = temp_value * 1.0000001 + 0.0000001;
                temp_value = temp_value / 0.9999999 - 0.0000001;
            }
            value = temp_value;
        }, 1);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}

static void BM_BigDataSize_DifficultTransform_MultiThread(benchmark::State& state) {
    std::vector<double> data(1000000, 1.0);
    for (auto _ : state) {
        auto values = data;
        ApplyFunction<double>(data, [](double& value) {
            double temp_value = value;
            for (int i = 0; i < 1000; ++i) {
                temp_value = temp_value * 1.0000001 + 0.0000001;
                temp_value = temp_value / 0.9999999 - 0.0000001;
            }
            value = temp_value;
        }, 10);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_SmallDataSize_EasyTransform_SingleThread)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SmallDataSize_EasyTransform_MultiThread)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_BigDataSize_DifficultTransform_SingleThread)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_BigDataSize_DifficultTransform_MultiThread)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();