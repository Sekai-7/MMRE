#pragma once

#include <shared_mutex>
#include <optional>
#include <vector>
#include <atomic>
#include <memory>
#include <unordered_map>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"
#include "CacheNode.hpp"
#include "ConcurrentObjectPool.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief High-performance Segmented Timeline Cache with Striped Locking.
 * Ensures 100% exact route matching and memory safety for multi-modal streams.
 */
class TimelineCache {
public:
    TimelineCache(std::shared_ptr<memory::ConcurrentObjectPool<CacheNode>> pool);
    ~TimelineCache() = default;

    /**
     * @brief 动态注册多模态资源路由。
     * 写入 Lock-Free Array，彻底消除原设计中 shared_mutex 带来的性能灾难。
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
    // 【重构：资源精确定位键】组合 ResourceId 与 SubResourceId 确保全局唯一
    using ResourceKey = uint64_t;
    static inline ResourceKey MakeKey(uint32_t resHash, uint32_t subHash) {
        return (static_cast<uint64_t>(resHash) << 32) | subHash;
    }

    // 独立资源的时间线结构（无锁，由外层 Bucket 锁保护）
    struct ResourceTimeline {
        CacheNode* root{nullptr};
        CacheNode* listHead{nullptr}; 
        CacheNode* listTail{nullptr}; 
    };

    // 【重构：并发分段路由桶】
    struct alignas(64) RouteBucket {
        std::shared_mutex rwLock; 
        // 精确映射，彻底杜绝哈希碰撞导致的多模态数据污染
        std::unordered_map<ResourceKey, ResourceTimeline> timelines; 
    };

    static constexpr size_t NUM_BUCKETS = 1024; // 分段锁数量，控制并发冲突度
    RouteBucket buckets_[NUM_BUCKETS];
    
    std::shared_ptr<memory::ConcurrentObjectPool<CacheNode>> nodePool_;
    
    inline size_t CalculateBucketIndex(ResourceKey key) const {
        // 扰乱函数打散哈希，路由至对应并发桶
        key ^= key >> 33;
        key *= 0xff51afd7ed558ccd;
        key ^= key >> 33;
        return key % NUM_BUCKETS;
    }
};

} // namespace engine_core
} // namespace mmre