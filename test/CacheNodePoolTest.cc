#include <gtest/gtest.h>
#include "CacheNodePool.h"
#include "common.h"
#include <vector>
#include <thread>
#include <set>
#include <mutex>
#include <atomic>
#include <random>
#include <algorithm>

// Helper to create a dummy packet
static UnifiedDataPacket createPacket(Timestamp ts, size_t size = 100) {
    return UnifiedDataPacket(ts, ResourceType::CAMERA, std::shared_ptr<void*>(), size);
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
    EXPECT_EQ(node->packet->type, ResourceType::CAMERA);
    
    // Check default initialization of tree fields
    EXPECT_EQ(node->left, nullptr);
    EXPECT_EQ(node->right, nullptr);
    EXPECT_EQ(node->parent, nullptr);
    EXPECT_EQ(node->height, 1);
    
    EXPECT_NO_THROW(pool.destroyNode(node));
}

// 验证复用时是否正确重置了对象状态（脏数据测试）
TEST_F(CacheNodePoolTest, DirtyReuse) {
    CacheNodePool pool(10);
    
    // 1. 分配一个节点
    CacheNode* node1 = pool.createNode(createPacket(100));
    void* addr1 = static_cast<void*>(node1);
    
    // 2. "弄脏" 它：模拟它曾在 AVL 树中使用过
    node1->left = reinterpret_cast<CacheNode*>(0xDEADBEEF);
    node1->right = reinterpret_cast<CacheNode*>(0xCAFEBABE);
    node1->parent = reinterpret_cast<CacheNode*>(0x12345678);
    node1->height = 99;

    // 3. 归还给池子
    pool.destroyNode(node1);
    
    // 4. 再次分配，预期拿到同一块内存（LIFO 特性）
    CacheNode* node2 = pool.createNode(createPacket(200));
    void* addr2 = static_cast<void*>(node2);
    
    EXPECT_EQ(addr1, addr2) << "Pool should reuse the most recently freed node (LIFO).";
    
    // 5. 关键检查：验证构造函数是否被再次调用，且字段被重置
    EXPECT_EQ(node2->timestamp, 200);
    EXPECT_EQ(node2->left, nullptr) << "Pointer 'left' was not reset!";
    EXPECT_EQ(node2->right, nullptr) << "Pointer 'right' was not reset!";
    EXPECT_EQ(node2->parent, nullptr) << "Pointer 'parent' was not reset!";
    EXPECT_EQ(node2->height, 1) << "Field 'height' was not reset!";
    
    pool.destroyNode(node2);
}

TEST_F(CacheNodePoolTest, SlabBoundaryAndExpansion) {
    size_t slabSize = 5;
    CacheNodePool pool(slabSize);
    std::vector<CacheNode*> nodes;
    std::set<CacheNode*> distinctNodes;

    // 1. Fill the first slab exactly
    for (size_t i = 0; i < slabSize; ++i) {
        CacheNode* n = pool.createNode(createPacket(i));
        ASSERT_NE(n, nullptr);
        nodes.push_back(n);
        distinctNodes.insert(n);
    }
    EXPECT_EQ(distinctNodes.size(), slabSize);

    // 2. Allocate ONE more to trigger expansion
    CacheNode* overflowNode = pool.createNode(createPacket(999));
    ASSERT_NE(overflowNode, nullptr);
    EXPECT_EQ(distinctNodes.count(overflowNode), 0) << "New slab node should be distinct from previous slab nodes";
    nodes.push_back(overflowNode);

    // 3. Clean up all
    for (auto* n : nodes) {
        pool.destroyNode(n);
    }
}

// 严格的多线程分配测试：确保所有线程同时持有内存时，池子能正确扩容，且无地址冲突
TEST_F(CacheNodePoolTest, MultiThreadedAllocation_Strict) {
    CacheNodePool pool(1000); 
    
    int numThreads = 10;
    int allocsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::mutex setMutex;
    std::set<CacheNode*> allAllocatedNodes;

    // 两个原子计数器作为屏障
    std::atomic<int> readyToStart{0};
    std::atomic<int> finishedAllocating{0}; // 用于 Alloc 和 Destroy 之间的同步
    std::atomic<bool> startGun{false};

    auto task = [&]() {
        // --- 阶段 1: 准备 ---
        readyToStart++;
        while (!startGun) std::this_thread::yield(); // 等待发令枪

        std::vector<CacheNode*> myNodes;
        myNodes.reserve(allocsPerThread);
        
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
        
        // --- 阶段 3: 中场休息 (Barrier) ---
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
    
    // 验证：必须分配出 totalNodes 个不重复的地址
    ASSERT_EQ(allAllocatedNodes.size(), numThreads * allocsPerThread) 
        << "Duplicate node allocation detected! Memory was reused too early or race condition occurred.";

    // 健康检查
    CacheNode* n = pool.createNode(createPacket(999));
    ASSERT_NE(n, nullptr);
    pool.destroyNode(n);
}

// 模拟真实业务场景：随机申请和释放（Churn），检测 Race Condition 导致的 Double Allocation
TEST_F(CacheNodePoolTest, ConcurrentChurnTest) {
    // 初始化池子，容量较小以强制频繁扩容和复用
    CacheNodePool pool(100); 
    
    int numThreads = 8;      // 线程数
    int iterations = 10000;  // 每个线程操作次数
    
    std::vector<std::thread> threads;
    
    // 全局“真理集合”：记录当前所有线程手中“正在持有”的节点
    std::set<CacheNode*> activeNodes;
    std::mutex checkMutex; // 专门保护测试验证逻辑的锁

    std::atomic<bool> failed{false}; // 标记测试是否失败

    auto task = [&](int threadId) {
        std::vector<CacheNode*> localNodes;
        
        // 每个线程独立的随机数引擎
        std::mt19937 gen(std::hash<std::thread::id>{}(std::this_thread::get_id()) + threadId);
        std::uniform_int_distribution<> actionDist(0, 1); // 0: alloc, 1: free

        for (int i = 0; i < iterations; ++i) {
            if (failed) return; // 只要有人报错，大家就停

            // 策略：如果本地是空的，必须借；如果太多，倾向于还
            bool doAlloc = (actionDist(gen) == 0);
            if (localNodes.empty()) doAlloc = true;
            if (localNodes.size() > 50) doAlloc = false; 

            if (doAlloc) {
                // --- 动作：申请 ---
                CacheNode* n = pool.createNode(createPacket(i));
                
                if (n == nullptr) {
                    ADD_FAILURE() << "Error: Got nullptr from pool!";
                    failed = true; return;
                }

                {
                    // === 关键验证 ===
                    std::lock_guard<std::mutex> lock(checkMutex);
                    // 如果这个节点已经在 activeNodes 里，说明它现在归别的线程所有！
                    // 而内存池竟然又把它分配给了我！-> 严重 Bug (Double Alloc)
                    if (activeNodes.count(n) > 0) {
                        ADD_FAILURE() << "CRITICAL ERROR: Double Allocation Detected! Node " << n << " is already in use.";
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
        threads.emplace_back(task, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(failed) << "Test failed due to Race Condition / Double Allocation.";
}

TEST_F(CacheNodePoolTest, MassiveExpansion) {
    // Start with a small slab size to force many expansions
    size_t slabSize = 100;
    CacheNodePool pool(slabSize);
    
    size_t totalNodes = slabSize * 10 + 50; // 10.5 slabs
    std::vector<CacheNode*> nodes;
    nodes.reserve(totalNodes);
    
    for (size_t i = 0; i < totalNodes; ++i) {
        CacheNode* n = pool.createNode(createPacket(i));
        ASSERT_NE(n, nullptr);
        nodes.push_back(n);
    }
    
    // Check all are valid and distinct
    std::set<CacheNode*> distinct(nodes.begin(), nodes.end());
    EXPECT_EQ(distinct.size(), totalNodes);
    
    // Check content of the last one
    EXPECT_EQ(nodes.back()->timestamp, (long long)(totalNodes - 1));
    
    // Free all
    for (auto* n : nodes) {
        pool.destroyNode(n);
    }
}

TEST_F(CacheNodePoolTest, PacketMoveSemantics) {
    CacheNodePool pool(10);
    Timestamp ts = 9999;
    size_t size = 1024;
    
    UnifiedDataPacket pkt(ts, ResourceType::GPS, std::shared_ptr<void*>(), size);
    
    // Verify move: createNode takes rvalue ref
    CacheNode* node = pool.createNode(std::move(pkt));
    
    ASSERT_NE(node, nullptr);
    ASSERT_NE(node->packet, nullptr);
    EXPECT_EQ(node->packet->timestamp, ts);
    EXPECT_EQ(node->packet->dataSize, size);
    EXPECT_EQ(node->packet->type, ResourceType::GPS);
    
    pool.destroyNode(node);
}

TEST_F(CacheNodePoolTest, ZeroSizeConstructor) {
    // Should default to 4096 (or whatever internal default is)
    // We just verify it doesn't crash and can allocate
    CacheNodePool pool(0);
    CacheNode* n = pool.createNode(createPacket(1));
    EXPECT_NE(n, nullptr);
    pool.destroyNode(n);
}