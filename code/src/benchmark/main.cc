#include <benchmark/benchmark.h>
#include <vector>

using std::vector;

static void benchmark_test(benchmark::State& state) {
    for (auto _ : state) {
        vector<int> vec;
        for (int i = 0; i < 100; ++i)
            vec.push_back(i);
    }
}

BENCHMARK(benchmark_test);

BENCHMARK_MAIN();