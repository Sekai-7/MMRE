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
     * @brief Returns a node to the free list for immediate reuse.
     */
    void Deallocate(engine_core::CacheNode* node);

private:
    struct Block {
        engine_core::CacheNode data;
        std::atomic<Block*> nextFree;
    };

    std::vector<Block*> allocatedChunks_;
    std::atomic<Block*> freeListHead_{nullptr};
    std::mutex growMutex_; // Only locks if initialCapacity is exhausted

    void GrowPool(size_t size);
};

} // namespace memory
} // namespace mmre