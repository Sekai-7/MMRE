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

/*
    CacheNodePool: 内存池，用于管理 CacheNode 对象的分配和释放。
    主要目的是：
    1. 避免频繁 new/delete 的开销，提高性能。
    2. 避免堆碎片问题。
    3. 提供 lock-free 分配/释放接口。
*/
// CacheNode 构造函数
CacheNode::CacheNode(const UnifiedDataPacket& p)
    : timestamp(p.timestamp), packet(p),
      parent(nullptr), left(nullptr), right(nullptr),
      height(1), prev_by_time(nullptr), next_by_time(nullptr) {}

struct CacheNodePool::Impl {
    std::atomic<CacheNode*> free_list{nullptr}; // lock-free LIFO 栈，用于快速分配节点
    std::vector<void*> slabs;                   // 保存所有 slab 的起始地址（mmap 返回的连续内存块）
    size_t nodes_per_slab;                      // 每个 slab 包含的 CacheNode 数量
    std::mutex slab_lock;                       // 仅在扩展 slab 时使用（避免多线程竞争）

    Impl(size_t nodes_per_slab_) : nodes_per_slab(nodes_per_slab_) {}

    ~Impl() {
        // 释放所有 mmap 分配的 slab
        for (void* p : slabs) {
            if (p) {
                size_t sz = nodes_per_slab * sizeof(CacheNode);
                munmap(p, sz);
            }
        }
    }
};

// -------------------- lock-free 辅助函数 --------------------

// 将节点压入 free list (无锁栈 LIFO)
static void push_free(std::atomic<CacheNode*>& head, CacheNode* node) {
    CacheNode* old_head = head.load(std::memory_order_relaxed);
    do {
        node->left = old_head; // 利用 left 指针存储 free list 的下一个节点
    } while (!head.compare_exchange_weak(old_head, node,
                                         std::memory_order_release,
                                         std::memory_order_relaxed));
}

// 从 free list 弹出节点 (无锁)
static CacheNode* pop_free(std::atomic<CacheNode*>& head) {
    CacheNode* old_head = head.load(std::memory_order_acquire);
    while (old_head) {
        CacheNode* next = old_head->left;
        if (head.compare_exchange_weak(old_head, next,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
            return old_head;
        }
    }
    return nullptr;
}

// -------------------- CacheNodePool 方法 --------------------

// 构造函数
CacheNodePool::CacheNodePool(size_t initial_nodes_per_slab) {
    // 使用 Impl 实现隐藏细节
    pimpl = new Impl(initial_nodes_per_slab ? initial_nodes_per_slab : 4096);

    // 分配第一块 slab
    std::lock_guard<std::mutex> lk(pimpl->slab_lock); // 避免多线程冲突
    size_t node_bytes = sizeof(CacheNode);
    size_t slab_bytes = pimpl->nodes_per_slab * node_bytes;

    void* slab = mmap(nullptr, slab_bytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (slab == MAP_FAILED) throw std::bad_alloc();
    pimpl->slabs.push_back(slab);

    // 构建 free list
    char* cur = static_cast<char*>(slab);
    for (size_t i = 0; i < pimpl->nodes_per_slab; ++i) {
        CacheNode* node = reinterpret_cast<CacheNode*>(cur + i * node_bytes);
        node->left = nullptr; // 初始化节点 left 指针
        push_free(pimpl->free_list, node);
    }
}

// 析构函数
CacheNodePool::~CacheNodePool() {
    delete pimpl; // 自动释放所有 slab
}

// 分配节点
CacheNode* CacheNodePool::createNode(const UnifiedDataPacket& packet) {
    // 尝试从 free list 弹出
    CacheNode* node = pop_free(pimpl->free_list);

    if (!node) {
        // free list 空，需要分配新的 slab
        std::lock_guard<std::mutex> lk(pimpl->slab_lock);
        node = pop_free(pimpl->free_list); // double-check

        if (!node) {
            size_t node_bytes = sizeof(CacheNode);
            size_t slab_bytes = pimpl->nodes_per_slab * node_bytes;
            void* slab = mmap(nullptr, slab_bytes, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (slab == MAP_FAILED) throw std::bad_alloc();
            pimpl->slabs.push_back(slab);

            // 将 slab 内的节点压入 free list
            char* cur = static_cast<char*>(slab);
            for (size_t i = 0; i < pimpl->nodes_per_slab; ++i) {
                CacheNode* slot = reinterpret_cast<CacheNode*>(cur + i * node_bytes);
                slot->left = nullptr;
                push_free(pimpl->free_list, slot);
            }

            node = pop_free(pimpl->free_list);
            assert(node != nullptr);
        }
    }

    // 使用 placement-new 构造 CacheNode
    CacheNode* constructed = new (node) CacheNode(packet);
    return constructed;
}

// 回收节点
void CacheNodePool::destroyNode(CacheNode* node) {
    if (!node) return;

    // 显式调用析构函数
    node->~CacheNode();

    // 压回 free list，可重复使用
    push_free(pimpl->free_list, node);
}
