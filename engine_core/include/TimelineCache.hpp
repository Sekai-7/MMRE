#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <optional>
#include <vector>
#include <atomic>
#include <memory>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"
#include "CacheNode.hpp"
#include "CacheNodePool.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief High-performance, SeqLock-enabled, segmented Timeline Cache.
 * Optimized for O(1) concurrent writes and O(1) Tx snapshot reads.
 */
class TimelineCache {
public:
    TimelineCache(std::shared_ptr<memory::CacheNodePool> pool);
    ~TimelineCache() = default;

    /**
     * @brief Pre-allocate routes for known resources to avoid runtime locking.
     */
    void RegisterResourceRoute(uint32_t resourceIdHash, uint32_t subResourceIdHash);

    /**
     * @brief Lock-free tail append for continuous streams. 
     * Takes payload by rvalue reference to strictly enforce ownership transfer (Zero-Copy).
     * Returns granular SystemStatus instead of void or bool.
     * @note Strictly meets the <= 1ms latency constraint.
     */
    common::SystemStatus Insert(common::TimestampNs ts, common::ResourceType type, 
                                uint32_t resHash, uint32_t subHash, 
                                uint32_t pluginSchemaId,
                                decltype(UnifiedDataPacket::payload)&& payload);

    /**
     * @brief O(1) Optimistic fast-path query for the latest snapshot (Tx).
     * Bypasses all standard mutexes using sequence locks.
     */
    std::optional<DataSnapshot> QueryLatest(uint32_t resHash, uint32_t subHash);

    /**
     * @brief O(logN) historical lookup for specific timestamp (Tx-n).
     */
    std::optional<DataSnapshot> QuerySnapshot(uint32_t resHash, uint32_t subHash, common::TimestampNs targetTime);

    /**
     * @brief O(logN + K) Range query. Falls back to shared_mutex.
     */
    std::vector<DataSnapshot> QueryRange(uint32_t resHash, uint32_t subHash, 
                                         common::TimestampNs startTime, common::TimestampNs endTime);

    /**
     * @brief Triggered by memory pressure, sweeps cold data to UFS.
     */
    void EvictColdData(uint64_t targetFreeBytes);

private:
    // Memory Barrier Sequence Lock for Lock-free Readers
    struct alignas(64) SeqLock {
        std::atomic<uint32_t> sequence{0};
        void lock_write() { sequence.fetch_add(1, std::memory_order_acquire); }
        void unlock_write() { sequence.fetch_add(1, std::memory_order_release); }
        uint32_t read_begin() const { return sequence.load(std::memory_order_acquire); }
        bool read_retry(uint32_t seq) const { 
            return (seq & 1) || seq != sequence.load(std::memory_order_acquire); 
        }
    };

    struct CacheSegment {
        SeqLock seqLock;             // Protects listTail for fast-path Tx reads
        std::shared_mutex treeLock;  // Protects RB-Tree for historical lookups & eviction
        
        CacheNode* root{nullptr};
        CacheNode* listHead{nullptr}; 
        std::atomic<CacheNode*> listTail{nullptr}; 
    };

    std::unordered_map<uint64_t, std::unique_ptr<CacheSegment>> segmentRoutes_;
    std::shared_ptr<memory::CacheNodePool> nodePool_;
    
    inline uint64_t CalculateRouteKey(uint32_t resHash, uint32_t subHash) const {
        return (static_cast<uint64_t>(resHash) << 32) | subHash;
    }
};

} // namespace engine_core
} // namespace mmre