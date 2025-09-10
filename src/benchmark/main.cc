#include <benchmark/benchmark.h>
#include <vector>

#include "TimelineCacheBenchmark.h"

using std::vector;

const int TOTAL_TIMES = 10000;

// sample code
static void sample_test(benchmark::State& state) {
    vector<int> vec;
    for (auto _ : state) {
        vec.clear();
        for (int i = 0; i < 100000; ++i)
            vec.push_back(i);
    }
}

static void benchmark_baseline_insert_test(benchmark::State& state) {
    char data[]{"data"};
    for (auto _ : state) {
        TimelineCacheBaseLine tcb{};
        for (int i = 0; i < 100000; ++i)
            tcb.insert(i, ResourceType::CAMERA, data, sizeof(data));
    }
}

static TimelineCacheBaseLine tcb;

static void benchmark_baseline_insert_thread_test(benchmark::State& state) {
    int threadNum = state.threads();
    int start = TOTAL_TIMES / threadNum;
    if (start = 0) {
        tcb.clear();
    }
    char data[]{"data"};
    for (auto _ : state) {
        for (int i = start; i < 100000; i += threadNum)
            tcb.insert(i, ResourceType::CAMERA, data, sizeof(data));
        state.PauseTiming();
        tcb.clear();
        state.ResumeTiming();
    }
}

static void benchmark_baseline_find_test(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        TimelineCacheBaseLine tcb{};
        char data[]{"data"};
        for (int i = 0; i < 100000; ++i)
            tcb.insert(i, ResourceType::CAMERA, data, sizeof(data));
        state.ResumeTiming();
        for (int i = 0; i < 100000; ++i)
            tcb.find(i);
    }
}

static CacheNodePool tcn;
static TimelineCache tc(tcn);

static void benchmark_avl_insert_thread_test(benchmark::State& state) {
    int threadNum = state.threads();
    int start = TOTAL_TIMES / threadNum;
    if (start = 0) {
        tcb.clear();
    }
    char data[]{"data"};
    for (auto _ : state) {
        for (int i = start; i < 100000; i += threadNum)
            tcb.insert(i, ResourceType::CAMERA, data, sizeof(data));
        state.PauseTiming();
        tc.clear();
        state.ResumeTiming();
    }
}

static void benchmark_avl_insert_test(benchmark::State& state) {
    for (auto _ : state) {
        CacheNodePool cnp{};
        TimelineCache tcb{cnp};
        char data[]{"data"};
        for (int i = 0; i < 100000; ++i)
            tcb.insert(i, UnifiedDataPacket{i, ResourceType::CAMERA, data, sizeof(data)});
    }
}

static void benchmark_avl_find_test(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        CacheNodePool cnp{};
        TimelineCache tcb{cnp};
        char data[]{"data"};
        for (int i = 0; i < 100000; ++i)
            tcb.insert(i, UnifiedDataPacket{i, ResourceType::CAMERA, data, sizeof(data)});
        state.ResumeTiming();
        for (int i = 0; i < 100000; ++i)
            tcb.find(i);
    }
}

BENCHMARK(benchmark_baseline_insert_test)->Unit(benchmark::kMillisecond);
BENCHMARK(benchmark_baseline_insert_thread_test)->Unit(benchmark::kMillisecond)->Threads(3);
BENCHMARK(benchmark_baseline_find_test)->Unit(benchmark::kMillisecond);
BENCHMARK(benchmark_avl_insert_test)->Unit(benchmark::kMillisecond);
BENCHMARK(benchmark_avl_insert_thread_test)->Unit(benchmark::kMillisecond)->Threads(3);
BENCHMARK(benchmark_avl_find_test)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();