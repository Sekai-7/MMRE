// CacheNodePool.cpp
// Code by LIU Lucia

#include "CacheNodePool.h"

#include <sys/mman.h>
#include <atomic>
#include <vector>
#include <mutex>

// /*
//     CacheNodePool: 内存池，用于管理 CacheNode 对象的分配和释放。
//     主要目的是：
//     1. 避免频繁 new/delete 的开销，提高性能。
//     2. 避免堆碎片问题。
//     3. 提供 lock-free 分配/释放接口。
// */
// // CacheNode 构造函数
CacheNode::CacheNode(UnifiedDataPacket&& p)
    : timestamp(p.timestamp), packet(new UnifiedDataPacket(std::move(p))),
      parent(nullptr), left(nullptr), right(nullptr),
      height(1) {}

struct CacheNodePool::Impl {
    std::vector<CacheNode*> freeStack;
    
    std::vector<void*> slabs;
    
    std::mutex poolLock; 
    
    size_t nodesPerSlab;

    Impl(size_t nodesPerSlab_) : nodesPerSlab(nodesPerSlab_) {
        freeStack.reserve(nodesPerSlab_);
    }

    ~Impl() {
        for (void* p : slabs) {
            if (p) {
                size_t sz = nodesPerSlab * sizeof(CacheNode);
                munmap(p, sz);
            }
        }
    }

    void expandSlab() {
        size_t nodeBytes = sizeof(CacheNode);
        size_t slabBytes = nodesPerSlab * nodeBytes;

        // mmap 分配内存
        void* slab = mmap(nullptr, slabBytes, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (slab == MAP_FAILED) {
            throw std::bad_alloc();
        }

        slabs.push_back(slab);

        // 切分内存并压入栈
        char* cur = static_cast<char*>(slab);
        for (size_t i = 0; i < nodesPerSlab; ++i) {
            CacheNode* node = reinterpret_cast<CacheNode*>(cur + i * nodeBytes);
            freeStack.push_back(node);
        }
    }
};

// -------------------- CacheNodePool 方法 --------------------

CacheNodePool::CacheNodePool(size_t initialNodesPerSlab) {
    impl = new Impl(initialNodesPerSlab ? initialNodesPerSlab : 4096);

    // 初始分配一块内存
    std::lock_guard<std::mutex> lock(impl->poolLock);
    impl->expandSlab();
}

CacheNodePool::~CacheNodePool() {
    delete impl;
}

CacheNode* CacheNodePool::createNode(UnifiedDataPacket&& packet) {
    CacheNode* memory = nullptr;

    {
        // --- 临界区开始 ---
        std::lock_guard<std::mutex> lock(impl->poolLock);

        // 如果栈空了，进行扩容
        if (impl->freeStack.empty()) {
            impl->expandSlab();
        }

        // 取出栈顶元素
        memory = impl->freeStack.back();
        impl->freeStack.pop_back();
        // --- 临界区结束 ---
    }

    return new (memory) CacheNode(std::move(packet));
}

void CacheNodePool::destroyNode(CacheNode* node) {
    if (!node) return;

    node->~CacheNode();

    {
        std::lock_guard<std::mutex> lock(impl->poolLock);
        impl->freeStack.push_back(node);
    }
}