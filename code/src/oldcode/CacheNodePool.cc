// CacheNodePool.cpp
// Code by LIU Lucia

#include "CacheNodePool.h"

#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <new>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

// /*
//     CacheNodePool: 内存池，用于管理 CacheNode 对象的分配和释放。
//     主要目的是：
//     1. 避免频繁 new/delete 的开销，提高性能。
//     2. 避免堆碎片问题。
//     3. 提供 lock-free 分配/释放接口。
// */
// // CacheNode 构造函数
CacheNode::CacheNode(UnifiedDataPacket&& p)
    : timestamp(p.timestamp), packet(std::move(p)),
      parent(nullptr), left(nullptr), right(nullptr),
      height(1) {}

struct CacheNodePool::Impl {
    std::atomic<CacheNode*> freeList{nullptr}; // lock-free LIFO 栈，用于快速分配节点
    std::vector<void*> slabs;                   // 保存所有 slab 的起始地址（mmap 返回的连续内存块）
    size_t nodesPerSlab;                      // 每个 slab 包含的 CacheNode 数量
    std::mutex slabLock;                       // 仅在扩展 slab 时使用（避免多线程竞争）

    Impl(size_t nodesPerSlab_) : nodesPerSlab(nodesPerSlab_) {}

    ~Impl() {
        // 释放所有 mmap 分配的 slab
        for (void* p : slabs) {
            if (p) {
                size_t sz = nodesPerSlab * sizeof(CacheNode);
                munmap(p, sz);
            }
        }
    }
};

// -------------------- lock-free 辅助函数 --------------------

// 将节点压入 free list (无锁栈 LIFO)
static void pushFree(std::atomic<CacheNode*>& head, CacheNode* node) {
    CacheNode* oldHead = head.load(std::memory_order_relaxed);
    do {
        node->left = oldHead; // 利用 left 指针存储 free list 的下一个节点
    } while (!head.compare_exchange_weak(oldHead, node,
                                         std::memory_order_release,
                                         std::memory_order_relaxed));
}

// 从 free list 弹出节点 (无锁)
static CacheNode* popFree(std::atomic<CacheNode*>& head) {
    CacheNode* oldHead = head.load(std::memory_order_acquire);
    while (oldHead) {
        CacheNode* next = oldHead->left;
        std::this_thread::yield();
        if (head.compare_exchange_weak(oldHead, next,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
            return oldHead;
        }
    }
    return nullptr;
}

// -------------------- CacheNodePool 方法 --------------------

// 构造函数
CacheNodePool::CacheNodePool(size_t initialNodesPerSlab) {
    // 使用 Impl 实现隐藏细节
    impl = new Impl(initialNodesPerSlab ? initialNodesPerSlab : 4096);

    // 分配第一块 slab
    std::lock_guard<std::mutex> lk(impl->slabLock); // 避免多线程冲突
    size_t nodeBytes = sizeof(CacheNode);
    size_t slabBytes = impl->nodesPerSlab * nodeBytes;

    void* slab = mmap(nullptr, slabBytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (slab == MAP_FAILED) throw std::bad_alloc();
    impl->slabs.push_back(slab);

    // 构建 free list
    char* cur = static_cast<char*>(slab);
    for (size_t i = 0; i < impl->nodesPerSlab; ++i) {
        CacheNode* node = reinterpret_cast<CacheNode*>(cur + i * nodeBytes);
        node->left = nullptr; // 初始化节点 left 指针
        pushFree(impl->freeList, node);
    }
}

// 析构函数
CacheNodePool::~CacheNodePool() {
    delete impl; // 自动释放所有 slab
}

// 分配节点
CacheNode* CacheNodePool::createNode(UnifiedDataPacket&& packet) {
    // 尝试从 free list 弹出
    CacheNode* node = popFree(impl->freeList);

    if (!node) {
        // free list 空，需要分配新的 slab
        std::lock_guard<std::mutex> lk(impl->slabLock);
        node = popFree(impl->freeList); // double-check

        if (!node) {
            size_t nodeBytes = sizeof(CacheNode);
            size_t slabBytes = impl->nodesPerSlab * nodeBytes;
            void* slab = mmap(nullptr, slabBytes, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (slab == MAP_FAILED) throw std::bad_alloc();
            impl->slabs.push_back(slab);

            // 将 slab 内的节点压入 free list
            char* cur = static_cast<char*>(slab);
            for (size_t i = 0; i < impl->nodesPerSlab; ++i) {
                CacheNode* slot = reinterpret_cast<CacheNode*>(cur + i * nodeBytes);
                slot->left = nullptr;
                pushFree(impl->freeList, slot);
            }

            node = popFree(impl->freeList);
            assert(node != nullptr);
        }
    }

    // 使用 placement-new 构造 CacheNode
    CacheNode* constructed = new (node) CacheNode(std::move(packet));
    return constructed;
}

// 回收节点
void CacheNodePool::destroyNode(CacheNode* node) {
    if (!node) return;

    // 显式调用析构函数
    node->~CacheNode();

    // 压回 free list，可重复使用
    pushFree(impl->freeList, node);
}