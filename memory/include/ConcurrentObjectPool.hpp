#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>

namespace mmre {
namespace memory {

/**
 * @brief Generic Lock-Free Slab Allocator with Deferred Reclamation (SMR).
 * 完全与业务类型 T 解耦，消除了依赖倒置，可复用于系统中任何需要 O(1) 分配的对象。
 */
template <typename T>
class ConcurrentObjectPool {
public:
    explicit ConcurrentObjectPool(size_t initialCapacity = 65536) {
        GrowPool(initialCapacity);
    }
    
    ~ConcurrentObjectPool() = default;

    T* Allocate() {
        Block* block = freeListHead_.load(std::memory_order_acquire);
        while (block && !freeListHead_.compare_exchange_weak(block, block->nextFree.load(std::memory_order_relaxed), 
                                                             std::memory_order_release, std::memory_order_relaxed)) {}
        if (!block) {
            std::lock_guard<std::mutex> lock(growMutex_);
            GrowPool(allocatedChunks_.empty() ? 65536 : allocatedChunks_.size() * 65536);
            return Allocate(); 
        }
        return &(block->data);
    }

    void Deallocate(T* node) {
        Block* block = reinterpret_cast<Block*>(node);
        Block* oldHead = freeListHead_.load(std::memory_order_relaxed);
        do {
            block->nextFree.store(oldHead, std::memory_order_relaxed);
        } while (!freeListHead_.compare_exchange_weak(oldHead, block, 
                                                      std::memory_order_release, std::memory_order_relaxed));
    }

    void DeferReclaim(T* node, uint64_t reclaimEpoch) {
        std::lock_guard<std::mutex> lock(deferredLock_);
        deferredQueue_.push_back({node, reclaimEpoch});
    }

    void SweepDeferredNodes(uint64_t safeEpoch) {
        std::lock_guard<std::mutex> lock(deferredLock_);
        auto it = deferredQueue_.begin();
        while (it != deferredQueue_.end()) {
            if (it->epoch <= safeEpoch) {
                Deallocate(it->node);
                it = deferredQueue_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    struct Block { T data; std::atomic<Block*> nextFree; };
    std::vector<Block*> allocatedChunks_;
    std::atomic<Block*> freeListHead_{nullptr};
    std::mutex growMutex_; 
    struct DeferredNode { T* node; uint64_t epoch; };
    std::mutex deferredLock_;
    std::vector<DeferredNode> deferredQueue_;

    void GrowPool(size_t size) { /* ... 内部扩容实现逻辑 ... */ }
};

} // namespace memory
} // namespace mmre