#include "TimelineCache.h"
#include "MultiResourceCheckpointManager.h"


#include <vector>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <optional>


class MultiResourceTimelineCache {
private:
    // 每种资源类型对应一个独立的 TimelineCache
    std::unordered_map<ResourceType, std::unique_ptr<TimelineCache>> resource_caches;

    // 共享内存管理器
    std::unique_ptr<SharedMemoryManager> shared_memory_manager;

    // 持久化管理
    std::unique_ptr<PersistenceEngine> persistence_engine;

    // 检查点管理器
    std::unique_ptr<MultiResourceCheckpointManager> checkpoint_manager;

    // 每个资源类型独立的读写锁
    mutable std::unordered_map<ResourceType, std::shared_mutex> resource_mutexes;
    std::shared_mutex resource_mutexes_lock; // 仅保护resource_mutexes本身

    // 缓存配置参数
    CacheConfiguration config;

public:
    explicit MultiResourceTimelineCache(const CacheConfiguration& cfg)
        : config(cfg)
        , shared_memory_manager(std::make_unique<SharedMemoryManager>())
        , checkpoint_manager(std::make_unique<MultiResourceCheckpointManager>())
        , persistence_engine(std::make_unique<PersistenceEngine>()) {
        //...正常初始化逻辑
        // ...
        // ...
        // 监测是否需要移除数据
        persistence_engine->set_monitored_cache(this);
        persistence_engine->start_monitoring();
    }

    // 插入数据
    // bool insert_data(ResourceType type, Timestamp timestamp, const RawResourceData& raw_data);
    bool insert(const UnifiedDataPacket&);

    // 查询数据
    QueryResult query_by_range(ResourceType type, Timestamp start_ts, Timestamp end_ts) const;

    MultiResourceQueryResult query_all_resources_by_range(Timestamp start_ts, Timestamp end_ts) const;

    // 立即持久化
    void trigger_immediate_persistence(ResourceType type);


    // 检查点相关代码
    // 插入检查点
    void insert_checkpoint(Timestamp timestamp);

    // 删除指定时间戳之前的检查点
    void remove_checkpoints_before(Timestamp timestamp);

    // 查询指定时间戳之前的检查点
    std::vector<Timestamp> query_checkpoints_before(Timestamp timestamp);

    // 查询指定时间范围内的检查点
    std::vector<Timestamp> query_checkpoints_in_range(Timestamp start, Timestamp end);

    // 查找最接近指定时间戳的检查点
    std::optional<Timestamp> find_nearest_checkpoint(Timestamp timestamp);

    // 获取最新检查点
    std::optional<Timestamp> get_latest_checkpoint();

    // 获取最早检查点
    std::optional<Timestamp> get_earliest_checkpoint();

    // 检查是否存在指定检查点
    bool has_checkpoint(Timestamp timestamp);

    // 获取检查点总数
    size_t get_checkpoint_count();

    // 清空所有检查点
    void clear_all_checkpoints();

    // 批量插入检查点
    void batch_insert_checkpoints(const std::vector<Timestamp>& timestamps);

    // 共享内存相关接口
    SharedMemoryHandle allocate_shared_memory(size_t size, ResourceType type);

    bool deallocate_shared_memory(const SharedMemoryHandle& handle);

    bool is_shared_memory_unreferenced(const SharedMemoryHandle& handle);

    // 获取共享内存引用计数
    int get_shared_memory_reference_count(const SharedMemoryHandle& handle);

    // 条件释放共享内存（仅当引用计数为0时释放）
    bool deallocate_shared_memory_if_unreferenced(const SharedMemoryHandle& handle);

    // 缓存管理接口
    bool enable_resource_cache(ResourceType type, bool enabled);

    CacheStatistics get_cache_statistics() const;

private:
    TimelineCache* get_or_create_cache(ResourceType type);

    UnifiedDataPacket convert_to_unified_packet(ResourceType type, Timestamp timestamp, const RawResourceData& raw_data) {
        // 根据资源类型和数据大小,决定是否使用共享内存存储
        // 并将原始数据转换为 UnifiedDataPacket
    }

    void notify_persistence_required(ResourceType type) {
        // 通知 PersistenceEngine 需要进行数据持久化
        // 可以通过回调或事件机制实现
    }

    // 检查指定范围是否完全被缓存覆盖
    bool is_range_completely_covered_by_cache(ResourceType type, Timestamp start_ts, Timestamp end_ts)；

    // 合并缓存查询结果和持久化查询结果
    QueryResult merge_query_results(const QueryResult& cache_result, 
                                   const QueryResult& persisted_result)；

};