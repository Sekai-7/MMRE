#include <gtest/gtest.h>
#include "CacheNodePool.h"
#include "common.h"
#include <vector>
#include <thread>

// Helper to create a dummy packet
static UnifiedDataPacket createPacket(Timestamp ts, size_t size = 100) {
    return UnifiedDataPacket(ts, ResourceType::CAMERA, nullptr, size);
}

class CacheNodePoolTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CacheNodePoolTest, Initialization) {
    // Test default constructor (default slab size)
    EXPECT_NO_THROW({
        CacheNodePool pool;
    });

    // Test constructor with specific slab size
    EXPECT_NO_THROW({
        CacheNodePool pool(100);
    });
}

TEST_F(CacheNodePoolTest, CreateAndDestroyNode) {
    CacheNodePool pool(10);
    Timestamp ts = 12345;
    
    CacheNode* node = pool.createNode(createPacket(ts));
    
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->timestamp, ts);
    EXPECT_EQ(node->packet.type, ResourceType::CAMERA);
    
    // Check default initialization of tree fields
    EXPECT_EQ(node->left, nullptr);
    EXPECT_EQ(node->right, nullptr);
    EXPECT_EQ(node->parent, nullptr);
    EXPECT_EQ(node->height, 1);
    
    EXPECT_NO_THROW(pool.destroyNode(node));
}

TEST_F(CacheNodePoolTest, ReuseMemory) {
    // Pool behaves as a LIFO stack for free nodes
    CacheNodePool pool(10);
    
    CacheNode* node1 = pool.createNode(createPacket(100));
    void* addr1 = static_cast<void*>(node1);
    
    pool.destroyNode(node1);
    
    CacheNode* node2 = pool.createNode(createPacket(200));
    void* addr2 = static_cast<void*>(node2);
    
    // In a LIFO implementation, we expect to get the same address back
    EXPECT_EQ(addr1, addr2);
    EXPECT_EQ(node2->timestamp, 200);
    
    pool.destroyNode(node2);
}

TEST_F(CacheNodePoolTest, SlabExpansion) {
    size_t slabSize = 5;
    CacheNodePool pool(slabSize);
    std::vector<CacheNode*> nodes;
    
    // Allocate more than one slab can hold
    int numToAllocate = slabSize * 3; 
    
    for (int i = 0; i < numToAllocate; ++i) {
        CacheNode* n = pool.createNode(createPacket(i));
        ASSERT_NE(n, nullptr);
        EXPECT_EQ(n->timestamp, i);
        nodes.push_back(n);
    }
    
    // Clean up
    for (auto* n : nodes) {
        pool.destroyNode(n);
    }
}

TEST_F(CacheNodePoolTest, MultiThreadedAllocation) {
    CacheNodePool pool(1000); // Sufficient size to reduce contention on slab expansion lock, though expansion is also thread safe
    
    int numThreads = 10;
    int allocsPerThread = 100;
    
    std::vector<std::thread> threads;
    
    auto task = [&]() {
        std::vector<CacheNode*> myNodes;
        for (int i = 0; i < allocsPerThread; ++i) {
            myNodes.push_back(pool.createNode(createPacket(i)));
        }
        
        // Simulate some work
        std::this_thread::yield();
        
        for (auto* n : myNodes) {
            pool.destroyNode(n);
        }
    };
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(task);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // After all threads are done, we should be able to allocate again cleanly
    CacheNode* n = pool.createNode(createPacket(999));
    ASSERT_NE(n, nullptr);
    pool.destroyNode(n);
}

TEST_F(CacheNodePoolTest, MultiThreadedAllocation_Strict) {
    CacheNodePool pool(1000); 
    
    int numThreads = 10;
    int allocsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::mutex setMutex;
    std::set<CacheNode*> allAllocatedNodes;

    // 两个原子计数器作为屏障
    std::atomic<int> readyToStart{0};
    std::atomic<int> finishedAllocating{0}; // <--- 新增：用于 Alloc 和 Destroy 之间的同步
    std::atomic<bool> startGun{false};

    auto task = [&]() {
        // --- 阶段 1: 准备 ---
        readyToStart++;
        while (!startGun); // 等待发令枪

        std::vector<CacheNode*> myNodes;
        
        // --- 阶段 2: 疯狂申请 ---
        for (int i = 0; i < allocsPerThread; ++i) {
            CacheNode* n = pool.createNode(createPacket(i));
            EXPECT_NE(n, nullptr); 
            if (n) {
                myNodes.push_back(n);
                std::lock_guard<std::mutex> lock(setMutex);
                allAllocatedNodes.insert(n);
            }
        }
        
        // --- 阶段 3: 中场休息 (关键修改点) ---
        // 我们必须等所有 10 个线程都申请完这 100 个节点，
        // 确保池子真的被掏空了 1000 个节点，而不是边申请边退还。
        finishedAllocating++;
        while (finishedAllocating < numThreads) {
             std::this_thread::yield(); // 自旋等待其他兄弟线程
        }

        // --- 阶段 4: 释放 ---
        for (auto* n : myNodes) {
            pool.destroyNode(n);
        }
    };
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(task);
    }

    // 主线程发令
    while(readyToStart < numThreads) std::this_thread::yield();
    startGun = true; 
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 现在的预期：
    // 因为我们在“释放”之前强行让所有线程都持有了内存，
    // 所以池子不得不扩容（或分配）出 1000 个不同的地址。
    ASSERT_EQ(allAllocatedNodes.size(), numThreads * allocsPerThread) 
        << "Duplicate node allocation detected! Memory was reused too early.";

    // 健康检查
    CacheNode* n = pool.createNode(createPacket(999));
    ASSERT_NE(n, nullptr);
    pool.destroyNode(n);
}

// TEST_F(CacheNodePoolTest, MultiThreadedAllocation_Strict) {
//     // 1. 容量给一点点余量，或者明确这是为了测扩容
//     CacheNodePool pool(1000); 
    
//     int numThreads = 10;
//     int allocsPerThread = 100;
    
//     std::vector<std::thread> threads;
//     std::mutex setMutex;
//     std::set<CacheNode*> allAllocatedNodes; // 用于检查唯一性

//     // C++20 Barrier (如果没有C++20，可以用 atomic counter 自旋等待)
//     // 作用：确保所有线程都就位了再一起开抢
//     std::atomic<int> readyCount{0};
//     std::atomic<bool> startGun{false};

//     auto task = [&]() {
//         // 等待所有线程就位
//         readyCount++;
//         while (!startGun); // 自旋等待发令枪

//         std::vector<CacheNode*> myNodes;
//         for (int i = 0; i < allocsPerThread; ++i) {
//             CacheNode* n = pool.createNode(createPacket(i));
            
//             // 严谨检查 1: 必须不能是空 (假设不支持扩容失败)
//             EXPECT_NE(n, nullptr); 

//             if (n) {
//                 myNodes.push_back(n);
//                 // 严谨检查 2: 记录地址，稍后检查唯一性
//                 std::lock_guard<std::mutex> lock(setMutex);
//                 allAllocatedNodes.insert(n);
//             }
//         }
        
//         // 释放逻辑
//         for (auto* n : myNodes) {
//             pool.destroyNode(n);
//         }
//     };
    
//     for (int i = 0; i < numThreads; ++i) {
//         threads.emplace_back(task);
//     }

//     // 主线程充当发令员
//     while(readyCount < numThreads) std::this_thread::yield();
//     startGun = true; // 砰！开始并发测试
    
//     for (auto& t : threads) {
//         t.join();
//     }
    
//     // 严谨检查 3: 确保发出去的 1000 个节点，地址全都不一样
//     // 如果 size < 1000，说明有两个线程抢到了同一个节点（严重Bug）
//     ASSERT_EQ(allAllocatedNodes.size(), numThreads * allocsPerThread) 
//         << "Duplicate node allocation detected! Race condition exists.";

//     // 最后的健康检查
//     CacheNode* n = pool.createNode(createPacket(999));
//     ASSERT_NE(n, nullptr);
//     pool.destroyNode(n);
// }

TEST_F(CacheNodePoolTest, ConcurrentChurnTest) {
    // 1. 初始化池子
    CacheNodePool pool(100); // 小一点，强制频繁扩容和复用
    
    int numThreads = 8;      // 线程数
    int iterations = 10000;  // 每个线程狂跑 1万次
    
    std::vector<std::thread> threads;
    
    // 全局“真理集合”：记录当前所有线程手中“正在持有”的节点
    std::set<CacheNode*> activeNodes;
    std::mutex checkMutex; // 专门保护测试验证逻辑的锁

    std::atomic<bool> failed{false}; // 标记测试是否失败

    auto task = [&]() {
        // 本地持有，模拟业务短暂使用
        std::vector<CacheNode*> localNodes; 
        
        for (int i = 0; i < iterations; ++i) {
            if (failed) return; // 只要有人报错，大家就停

            // 随机决定是“借”还是“还”
            // 保持一定的持有量，比如 50% 概率借，50% 概率还
            bool doAlloc = (rand() % 2 == 0);
            if (localNodes.empty()) doAlloc = true;
            if (localNodes.size() > 50) doAlloc = false; // 别借太多，强迫归还复用

            if (doAlloc) {
                // --- 动作：申请 ---
                CacheNode* n = pool.createNode(createPacket(i));
                
                if (n == nullptr) {
                    // 理论上只要内存够，不该返回空
                    printf("Error: Got nullptr!\n");
                    failed = true; return;
                }

                {
                    // === 关键验证 ===
                    std::lock_guard<std::mutex> lock(checkMutex);
                    // 如果这个节点已经在 activeNodes 里，说明它现在归别的线程所有！
                    // 而内存池竟然又把它分配给了我！-> 严重 Bug (Double Alloc)
                    if (activeNodes.count(n) > 0) {
                        printf("CRITICAL ERROR: Double Allocation Detected! Node %p is already in use.\n", n);
                        failed = true; return;
                    }
                    activeNodes.insert(n);
                }
                localNodes.push_back(n);

            } else {
                // --- 动作：释放 ---
                CacheNode* n = localNodes.back();
                localNodes.pop_back();

                {
                    std::lock_guard<std::mutex> lock(checkMutex);
                    activeNodes.erase(n); // 从“在用名单”里划掉
                }
                
                // 归还给池子
                pool.destroyNode(n);
            }
        }

        // 跑完后把剩下的都还了
        for (auto* n : localNodes) {
             std::lock_guard<std::mutex> lock(checkMutex);
             activeNodes.erase(n);
             pool.destroyNode(n);
        }
    };

    // 启动线程
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(task);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(failed) << "Test failed due to Race Condition / Double Allocation.";
}
