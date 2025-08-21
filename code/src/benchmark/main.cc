#include <benchmark/benchmark.h>
#include <vector>

#include "TimelineCacheBenchmark.h"

using std::vector;

// sample code
static void sample_test(benchmark::State& state) {
    vector<int> vec;
    for (auto _ : state) {
        vec.clear();
        for (int i = 0; i < 10000; ++i)
            vec.push_back(i);
    }
}

static void benchmark_insert_test(benchmark::State& state) {
    for (auto _ : state) {
        TimelineCacheBaseLine tcb{};
        char data[]{"data"};
        for (int i = 0; i < 10000; ++i)
            tcb.insert(i, DataType::CAMERA, data, sizeof(data));
    }
}

static void benchmark_find_test(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        TimelineCacheBaseLine tcb{};
        char data[]{"data"};
        for (int i = 0; i < 10000; ++i)
            tcb.insert(i, DataType::CAMERA, data, sizeof(data));
        state.ResumeTiming();
        for (int i = 0; i < 10000; ++i)
            tcb.find(i);
    }
}

BENCHMARK(benchmark_insert_test)->Unit(benchmark::kMillisecond);
BENCHMARK(benchmark_find_test)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();