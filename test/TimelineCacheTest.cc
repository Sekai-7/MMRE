#include <gtest/gtest.h>
#include "TimelineCache.h"
#include "common.h"
#include <climits>

// Helper to create a dummy packet
UnifiedDataPacket createPacket(Timestamp ts, size_t size = 100) {
    return UnifiedDataPacket(ts, ResourceType::CAMERA, nullptr, size);
}

class TimelineCacheTest : public ::testing::Test {
protected:
    TimelineCache cache;

    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
        cache.clear();
    }
};

TEST_F(TimelineCacheTest, Initialization) {
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.memoryUsage(), 0);
    EXPECT_EQ(cache.getMinTimestamp(), LLONG_MAX);
    EXPECT_EQ(cache.getMaxTimestamp(), LLONG_MIN);
}

TEST_F(TimelineCacheTest, InsertSingle) {
    Timestamp ts = 1000;
    size_t size = 50;
    cache.insert(createPacket(ts, size));

    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.memoryUsage(), size);
    EXPECT_EQ(cache.getMinTimestamp(), ts);
    EXPECT_EQ(cache.getMaxTimestamp(), ts);

    UnifiedDataPacket* p = cache.query(ts);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->timestamp, ts);
}

TEST_F(TimelineCacheTest, InsertMultipleOrdering) {
    cache.insert(createPacket(100, 10));
    cache.insert(createPacket(50, 10));
    cache.insert(createPacket(150, 10));

    EXPECT_EQ(cache.size(), 3);
    EXPECT_EQ(cache.memoryUsage(), 30);
    EXPECT_EQ(cache.getMinTimestamp(), 50);
    EXPECT_EQ(cache.getMaxTimestamp(), 150);
}

TEST_F(TimelineCacheTest, InsertDuplicateOverwrite) {
    cache.insert(createPacket(100, 10));
    cache.insert(createPacket(100, 20)); // Overwrite with larger size

    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.memoryUsage(), 20); // Should update size
    
    UnifiedDataPacket* p = cache.query(100);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->dataSize, 20);
}

TEST_F(TimelineCacheTest, QueryRange) {
    for (int i = 0; i < 10; ++i) {
        cache.insert(createPacket(i * 10, 10));
    }
    // 0, 10, 20, ... 90

    auto res = cache.queryByRange(20, 50);
    EXPECT_EQ(res.size(), 4); // 20, 30, 40, 50
    
    // Check sorted order
    if (!res.empty()) {
        for (size_t i = 0; i < res.size() - 1; ++i) {
             EXPECT_LE(res[i]->timestamp, res[i+1]->timestamp);
        }
    }
}

TEST_F(TimelineCacheTest, Remove) {
    cache.insert(createPacket(10, 10));
    cache.insert(createPacket(20, 10));
    cache.insert(createPacket(30, 10));

    EXPECT_TRUE(cache.remove(20));
    EXPECT_EQ(cache.size(), 2);
    EXPECT_EQ(cache.query(20), nullptr);
    
    EXPECT_FALSE(cache.remove(999)); // Non-existent
    EXPECT_EQ(cache.size(), 2);
    
    EXPECT_TRUE(cache.remove(10)); // Min
    EXPECT_EQ(cache.getMinTimestamp(), 30);
    
    EXPECT_TRUE(cache.remove(30)); // Max/Last
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.getMinTimestamp(), LLONG_MAX);
}

TEST_F(TimelineCacheTest, Clear) {
    cache.insert(createPacket(10, 10));
    cache.insert(createPacket(20, 10));
    cache.clear();
    
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.memoryUsage(), 0);
    EXPECT_EQ(cache.query(10), nullptr);
    EXPECT_EQ(cache.getMinTimestamp(), LLONG_MAX);
}

TEST_F(TimelineCacheTest, AVLRothations) {
    // Force right heavy to cause left rotation
    // Insert 10, 20, 30
    cache.insert(createPacket(10, 10));
    cache.insert(createPacket(20, 10));
    cache.insert(createPacket(30, 10));
    
    // Root should be 20 if balanced correctly
    // But we can't inspect root directly easily without friendship or accessors.
    // We can just verify it works.
    EXPECT_EQ(cache.size(), 3);
    EXPECT_NE(cache.query(10), nullptr);
    EXPECT_NE(cache.query(20), nullptr);
    EXPECT_NE(cache.query(30), nullptr);
}

TEST_F(TimelineCacheTest, MultiThreadedInsert) {
    int numThreads = 4;
    int insertsPerThread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, insertsPerThread]() {
            for (int j = 0; j < insertsPerThread; ++j) {
                // Ensure unique timestamps across threads to avoid overwrites
                Timestamp ts = i * insertsPerThread + j; 
                cache.insert(createPacket(ts, 10));
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    EXPECT_EQ(cache.size(), numThreads * insertsPerThread);
}