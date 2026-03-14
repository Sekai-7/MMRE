#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>
#include "CacheNode.hpp"

namespace mmre {
namespace memory {

/**
 * @brief Lock-free Slab Allocator strictly designed for CacheNode.
 * Ensures O(1) allocation latency for high-frequency Vehicle Signals.
 */
class CacheNodePool {
public:
    explicit CacheNodePool(size_t initialCapacity = 65536);
    ~CacheNodePool();

    /**
     * @brief Acquires a clean, uninitialized node from the free list.
     */
    engine_core::CacheNode* Allocate();

    /**
     * @brief Return a node to the pool IMMEDIATELY (Only safe if strictly single-threaded).
     */
    void Deallocate(engine_core::CacheNode* node);

    /**
     * @brief 基于安全内存回收(SMR)的延迟释放机制。
     * 防止 TimelineCache 的 Lock-Free 读线程发生 Use-After-Free 崩溃。
     * @param node 待回收的节点
     * @param reclaimEpoch 触发回收时的全局时间戳或事务纪元
     */
    void DeferReclaim(engine_core::CacheNode* node, uint64_t reclaimEpoch);

    /**
     * @brief 由后台 GC 线程定期调用，清理确实已经没有读取线程占用的 Node。
     */
    void SweepDeferredNodes(uint64_t safeEpoch);

private:
    struct Block {
        engine_core::CacheNode data;
        std::atomic<Block*> nextFree;
    };

    std::vector<Block*> allocatedChunks_;
    std::atomic<Block*> freeListHead_{nullptr};
    std::mutex growMutex_; // Only locks if initialCapacity is exhausted

    struct DeferredNode {
        engine_core::CacheNode* node;
        uint64_t epoch;
    };
    std::mutex deferredLock_;
    std::vector<DeferredNode> deferredQueue_;

    void GrowPool(size_t size);
};

} // namespace memory
} // namespace mmre