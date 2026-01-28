#include <gtest/gtest.h>
#include "TimelineCache.h"
#include "common.h"
#include <climits>
#include <memory>

// Helper to create a dummy packet with timestamp encoded in Handle
static UnifiedDataPacket createPacket(Timestamp ts, size_t size = 100) {
    // We allocate a void* on the heap to store the timestamp (casted to void*)
    // This is to conform to shared_ptr<void*> signature.
    // Note: This is a hack to fit the type std::shared_ptr<void*> which manages a void**.
    // The variant expects std::shared_ptr<void*>.
    // So we need a pointer to void* that is managed.
    
    void** ptrStorage = new void*(reinterpret_cast<void*>(ts));
    std::shared_ptr<void*> handle(ptrStorage); 
    return UnifiedDataPacket(ts, ResourceType::CAMERA, handle, size);
}

// Helper to extract timestamp from Handle
static Timestamp getTimestamp(const Handle& h) {
    if (auto* sp = std::get_if<std::shared_ptr<void*>>(&h)) {
        if (*sp && *(*sp)) { // Check shared_ptr valid and the void* it points to
             return reinterpret_cast<Timestamp>(*(*sp));
        }
        // If *sp is valid but points to nullptr/0 (valid TS 0), return it.
        if (*sp) return reinterpret_cast<Timestamp>(*(*sp));
    }
    return -1; // Error or invalid
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

    Handle h = cache.query(ts);
    // Check if valid handle
    EXPECT_EQ(getTimestamp(h), ts);
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
    
    Handle h = cache.query(100);
    EXPECT_EQ(getTimestamp(h), 100);
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
        Timestamp prev = -1;
        for (const auto& h : res) {
            Timestamp curr = getTimestamp(h);
            if (prev != -1) {
                EXPECT_LE(prev, curr);
            }
            prev = curr;
        }
    }
}

TEST_F(TimelineCacheTest, Remove) {
    cache.insert(createPacket(10, 10));
    cache.insert(createPacket(20, 10));
    cache.insert(createPacket(30, 10));

    EXPECT_TRUE(cache.remove(20));
    EXPECT_EQ(cache.size(), 2);
    
    // Check it's gone
    Handle h = cache.query(20);
    // Assuming query returns null/empty handle if not found.
    // Based on implementation returning nullptr (which converts to null shared_ptr)
    if (auto* sp = std::get_if<std::shared_ptr<void*>>(&h)) {
        EXPECT_EQ(*sp, nullptr);
    }
    
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
    
    Handle h = cache.query(10);
    if (auto* sp = std::get_if<std::shared_ptr<void*>>(&h)) {
        EXPECT_EQ(*sp, nullptr);
    }
    
    EXPECT_EQ(cache.getMinTimestamp(), LLONG_MAX);
}

TEST_F(TimelineCacheTest, AVLRothations) {
    // Force right heavy to cause left rotation
    // Insert 10, 20, 30
    cache.insert(createPacket(10, 10));
    cache.insert(createPacket(20, 10));
    cache.insert(createPacket(30, 10));
    
    EXPECT_EQ(cache.size(), 3);
    EXPECT_EQ(getTimestamp(cache.query(10)), 10);
    EXPECT_EQ(getTimestamp(cache.query(20)), 20);
    EXPECT_EQ(getTimestamp(cache.query(30)), 30);
}

// TEST_F(TimelineCacheTest, MultiThreadedInsert) {
//     int numThreads = 4;
//     int insertsPerThread = 100;
//     std::vector<std::thread> threads;

//     for (int i = 0; i < numThreads; ++i) {
//         threads.emplace_back([this, i, insertsPerThread]() {
//             for (int j = 0; j < insertsPerThread; ++j) {
//                 // Ensure unique timestamps across threads to avoid overwrites
//                 Timestamp ts = i * insertsPerThread + j; 
//                 cache.insert(createPacket(ts, 10));
//             }
//         });
//     }

//     for (auto& t : threads) {
//         if (t.joinable()) {
//             t.join();
//         }
//     }

//     EXPECT_EQ(cache.size(), numThreads * insertsPerThread);
// }