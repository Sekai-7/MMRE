#ifndef MULTIRESOURCETIMELINECACHE_H
#define MULTIRESOURCETIMELINECACHE_H

#include "TimelineCache.h"
#include "MultiResourceCheckpointManager.h"
#include "Query.h"


#include <vector>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

// 暂时确保程序能跑
class CacheConfiguration {

};

class PersistenceEngine {

};

using MultiResourceQueryResult = std::vector<QueryResult>;

class MultiResourceTimelineCache {
private:
    // 每种资源类型对应一个独立的 TimelineCache
    std::unordered_map<ResourceType, std::unique_ptr<TimelineCache>> resourceCaches;

    // 共享内存管理器
    std::unique_ptr<SharedMemoryManager> sharedMemoryManager;

    // 持久化管理
    std::unique_ptr<PersistenceEngine> persistenceEngine;

    // 检查点管理器
    std::unique_ptr<MultiResourceCheckpointManager> checkpointManager;

    // 每个资源类型独立的读写锁
    mutable std::unordered_map<ResourceType, std::shared_mutex> resourceMutexes;
    std::shared_mutex resourceMutexesLock; // 仅保护resourceMutexes本身

    // 缓存配置参数
    CacheConfiguration config;

public:
    explicit MultiResourceTimelineCache(const CacheConfiguration& cfg)
        : config(cfg)
        , sharedMemoryManager(std::make_unique<SharedMemoryManager>())
        , checkpointManager(std::make_unique<MultiResourceCheckpointManager>())
        , persistenceEngine(std::make_unique<PersistenceEngine>()) {
        //...正常初始化逻辑
        // ...
        // ...
        // 监测是否需要移除数据
        // persistenceEngine->setMonitoredCache(this);
        // persistenceEngine->startMonitoring();
    }

    // 插入数据
    bool insert(UnifiedDataPacket&&);

    // 查询数据
    QueryResult queryByRange(ResourceType type, Timestamp startTs, Timestamp endTs) const;

    QueryResult query(ResourceType type, Timestamp ts) const;

    MultiResourceQueryResult queryAllResourcesByRange(Timestamp startTs, Timestamp endTs) const;

    // 立即持久化
    void triggerImmediatePersistence(ResourceType type);


    // 检查点相关代码
    // 插入检查点
    void insertCheckpoint(Timestamp timestamp);

    // 删除指定时间戳之前的检查点
    void removeCheckpointsBefore(Timestamp timestamp);

    // 查询指定时间戳之前的检查点
    std::vector<Timestamp> queryCheckpointsBefore(Timestamp timestamp);

    // 查询指定时间范围内的检查点
    std::vector<Timestamp> queryCheckpointsInRange(Timestamp start, Timestamp end);

    // 查找最接近指定时间戳的检查点
    std::optional<Timestamp> findNearestCheckpoint(Timestamp timestamp);

    // 获取最新检查点
    std::optional<Timestamp> getLatestCheckpoint();

    // 获取最早检查点
    std::optional<Timestamp> getEarliestCheckpoint();

    // 检查是否存在指定检查点
    bool hasCheckpoint(Timestamp timestamp);

    // 获取检查点总数
    size_t getCheckpointCount();

    // 清空所有检查点
    void clearAllCheckpoints();

    // 批量插入检查点
    void batchInsertCheckpoints(const std::vector<Timestamp>& timestamps);

    // 共享内存相关接口
    SharedMemoryHandle allocateSharedMemory(size_t size, ResourceType type);

    bool deallocateSharedMemory(const SharedMemoryHandle& handle);

    bool isSharedMemoryUnreferenced(const SharedMemoryHandle& handle);

    // 获取共享内存引用计数
    int getSharedMemoryReferenceCount(const SharedMemoryHandle& handle);

    // 条件释放共享内存（仅当引用计数为0时释放）
    bool deallocateSharedMemoryIfUnreferenced(const SharedMemoryHandle& handle);

    // 缓存管理接口
    bool enableResourceCache(ResourceType type, bool enabled);

    // CacheStatistics getCacheStatistics() const;

private:
    TimelineCache* getOrCreateCache(ResourceType type);

    // UnifiedDataPacket convertToUnifiedPacket(ResourceType type, Timestamp timestamp, const RawResourceData& rawData) {
        // 根据资源类型和数据大小,决定是否使用共享内存存储
        // 并将原始数据转换为 UnifiedDataPacket
    // }

    // void notifyPersistenceRequired(ResourceType type) {
        // 通知 PersistenceEngine 需要进行数据持久化
        // 可以通过回调或事件机制实现
    // }

    // 检查指定范围是否完全被缓存覆盖
    // bool isRangeCompletelyCoveredByCache(ResourceType type, Timestamp startTs, Timestamp endTs);

    // 合并缓存查询结果和持久化查询结果
    // QueryResult mergeQueryResults(const QueryResult& cacheResult, const QueryResult& persistedResult);

};

#endif