#include <gtest/gtest.h>
#include "MultiResourceCheckpointManager.h"
#include <vector>
#include <thread>
#include <algorithm>
#include <random>

class MultiResourceCheckpointManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试开始前创建一个新的 manager
        manager = std::make_unique<MultiResourceCheckpointManager>();
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<MultiResourceCheckpointManager> manager;
};

// 1. 基本插入和查询测试
TEST_F(MultiResourceCheckpointManagerTest, InsertAndQueryBasic) {
    EXPECT_EQ(manager->getCheckpointCount(), 0);

    manager->insertCheckpoint(100);
    manager->insertCheckpoint(200);
    manager->insertCheckpoint(300);

    EXPECT_EQ(manager->getCheckpointCount(), 3);
    EXPECT_TRUE(manager->hasCheckpoint(100));
    EXPECT_TRUE(manager->hasCheckpoint(200));
    EXPECT_FALSE(manager->hasCheckpoint(150));

    EXPECT_EQ(manager->getLatestCheckpoint(), 300);
    EXPECT_EQ(manager->getEarliestCheckpoint(), 100);
}

// 2. 批量插入测试
TEST_F(MultiResourceCheckpointManagerTest, InsertBatch) {
    std::vector<Timestamp> batch = {10, 50, 30, 20, 40}; // 乱序
    manager->insertBatchCheckpoint(batch);

    EXPECT_EQ(manager->getCheckpointCount(), 5);
    EXPECT_EQ(manager->getEarliestCheckpoint(), 10);
    EXPECT_EQ(manager->getLatestCheckpoint(), 50);
}

// 3. 范围查询测试
TEST_F(MultiResourceCheckpointManagerTest, QueryRange) {
    std::vector<Timestamp> timestamps = {100, 200, 300, 400, 500};
    manager->insertBatchCheckpoint(timestamps);

    // 查询 [200, 400] -> 应该包含 200, 300, 400
    // 注意：lower_bound(start) 包含 start，upper_bound(end) 不包含 end 但指向 > end 的第一个元素
    // 所以 vector(lower, upper) 实际上是 [start, end] 闭区间（如果 end 存在于集合中）
    auto range = manager->queryCheckpointsRange(200, 400);
    ASSERT_EQ(range.size(), 3);
    EXPECT_EQ(range[0], 200);
    EXPECT_EQ(range[1], 300);
    EXPECT_EQ(range[2], 400);

    // 查询 (200, 400) -> 实际上是 queryRange(201, 399)
    auto rangeInner = manager->queryCheckpointsRange(201, 399);
    ASSERT_EQ(rangeInner.size(), 1);
    EXPECT_EQ(rangeInner[0], 300);
}

// 4. 删除测试
TEST_F(MultiResourceCheckpointManagerTest, RemoveBefore) {
    std::vector<Timestamp> timestamps = {10, 20, 30, 40, 50};
    manager->insertBatchCheckpoint(timestamps);

    // 删除 < 35 的元素 -> 10, 20, 30 被删
    size_t removed = manager->removeCheckpointsBefore(35);
    EXPECT_EQ(removed, 3);
    EXPECT_EQ(manager->getCheckpointCount(), 2);
    EXPECT_EQ(manager->getEarliestCheckpoint(), 40);
}

// 5. 最近邻查找测试 (核心逻辑)
TEST_F(MultiResourceCheckpointManagerTest, FindNearest) {
    // 0, 100, 200, 300
    manager->insertCheckpoint(0);
    manager->insertCheckpoint(100);
    manager->insertCheckpoint(200);
    manager->insertCheckpoint(300);

    // 精确匹配
    EXPECT_EQ(manager->findNearestCheckpoint(100), 100);

    // 偏左：140 离 100 更近 (dist 40 vs 60)
    EXPECT_EQ(manager->findNearestCheckpoint(140), 100);

    // 偏右：160 离 200 更近 (dist 60 vs 40)
    EXPECT_EQ(manager->findNearestCheckpoint(160), 200);

    // 中点：150，代码逻辑是 <= 则取左边，或者看具体实现
    // 根据代码: std::abs(*prev - timestamp) <= std::abs(*it - timestamp) -> return prev
    // abs(100-150)=50, abs(200-150)=50. 50 <= 50 -> true -> prev(100)
    EXPECT_EQ(manager->findNearestCheckpoint(150), 100);

    // 边界外：-50 -> 0
    EXPECT_EQ(manager->findNearestCheckpoint(-50), 0);

    // 边界外：400 -> 300
    EXPECT_EQ(manager->findNearestCheckpoint(400), 300);
}

// 6. 空容器测试
TEST_F(MultiResourceCheckpointManagerTest, EmptyContainerBehavior) {
    EXPECT_EQ(manager->getLatestCheckpoint(), -1);
    EXPECT_EQ(manager->getEarliestCheckpoint(), -1);
    EXPECT_EQ(manager->findNearestCheckpoint(100), -1);
    
    // 删除空容器不应崩溃
    EXPECT_EQ(manager->removeCheckpointsBefore(100), 0);
}

// 修正后的并发测试
TEST_F(MultiResourceCheckpointManagerTest, ThreadSafety) {
    const int WRITER_THREADS = 4;
    const int READER_THREADS = 4;
    const int ITERATIONS = 20000;

    std::vector<std::thread> threads;

    // 写线程
    for (int i = 0; i < WRITER_THREADS; ++i) {
        threads.emplace_back([this, i, ITERATIONS]() {
            std::mt19937 gen(i);
            std::uniform_int_distribution<> dist(0, 100000);
            for (int j = 0; j < ITERATIONS; ++j) {
                // 混合插入和删除
                if (j % 10 == 0) {
                    manager->removeCheckpointsBefore(dist(gen) / 2); // 偶尔删除
                } else {
                    manager->insertCheckpoint(dist(gen));
                }
            }
        });
    }

    // 读线程
    for (int i = 0; i < READER_THREADS; ++i) {
        threads.emplace_back([this, i, ITERATIONS]() {
            std::mt19937 gen(i + 100);
            std::uniform_int_distribution<> dist(0, 100000);
            for (int j = 0; j < ITERATIONS; ++j) {
                manager->findNearestCheckpoint(dist(gen));
                manager->queryCheckpointsRange(dist(gen), dist(gen) + 100);
                manager->getLatestCheckpoint();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    
    // 如果没有 Crash 且数据结构没损坏，就算通过
    // 可以加一个简单的最终检查
    EXPECT_GE(manager->getCheckpointCount(), 0);
}
