// Code by LIU Lucia

#include <iostream>
#include <vector>
#include <chrono>

#include "TimelineCacheBenchmark.h"

using namespace std;
using namespace std::chrono;

// 统一规模定义
constexpr int TEST_SIZE = 100000;  // 可以改成 10万、50万、100万等

// 工具函数：执行并计时
template <typename Func>
void run_benchmark(const string& name, Func&& f, int repeat = 5) {
    double total = 0.0;
    for (int r = 0; r < repeat; ++r) {
        auto start = high_resolution_clock::now();
        f();
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        total += duration;
        cout << name << " run " << r + 1 << ": " << duration << " ms" << endl;
    }
    cout << name << " avg: " << (total / repeat) << " ms" << endl << endl;
}

// ------------------- 测试用例 -------------------

void sample_test() {
    vector<int> vec;
    vec.reserve(TEST_SIZE); // 避免多次 realloc
    for (int i = 0; i < TEST_SIZE; ++i)
        vec.push_back(i);
}

void baseline_insert_test() {
    TimelineCacheBaseLine tcb{};
    char data[]{"data"};
    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.insert(i, DataType::CAMERA, data, sizeof(data));
}

void baseline_find_test() {
    TimelineCacheBaseLine tcb{};
    char data[]{"data"};
    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.insert(i, DataType::CAMERA, data, sizeof(data));

    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.find(i);
}

void avl_insert_test() {
    CacheNodePoolBenchmark cnp{};
    TimelineCacheBenchmark tcb{cnp};
    char data[]{"data"};
    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.insert(i, UnifiedDataPacketBenchmark{i, DataType::CAMERA, data, sizeof(data)});
}

void avl_find_test() {
    CacheNodePoolBenchmark cnp{};
    TimelineCacheBenchmark tcb{cnp};
    char data[]{"data"};
    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.insert(i, UnifiedDataPacketBenchmark{i, DataType::CAMERA, data, sizeof(data)});

    for (int i = 0; i < TEST_SIZE; ++i)
        tcb.find(i);
}

// ------------------- main -------------------

int main() {
    run_benchmark("sample_test", sample_test);
    run_benchmark("baseline_insert_test", baseline_insert_test);
    run_benchmark("baseline_find_test", baseline_find_test);
    run_benchmark("avl_insert_test", avl_insert_test);
    run_benchmark("avl_find_test", avl_find_test);
    return 0;
}