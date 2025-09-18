#include "MultiResourceTimelineCache.h"

bool MultiResourceTimelineCache::insert(const UnifiedDataPacket& data) {
    TimelineCache* cache = get_or_create_cache(data.type);
    if (cache == nullptr)
        return false;
    cache->insert(data);
    return true;
}

// 查询数据
QueryResult MultiResourceTimelineCache::query_by_range(ResourceType type, Timestamp start_ts, Timestamp end_ts) const
{
    QueryResult result;

    // 1. 查找资源类型的缓存
    auto it = resource_caches.find(type);
    if (it == resource_caches.end()) {
        return result; // 返回空结果
    }

    // 2. 加锁保护
    std::shared_lock<std::shared_mutex> lock(resource_mutexes.at(type));

    // 3. 查询缓存数据
    TimelineCache *cache = it->second.get();
    if (cache) {
        result.resource_data.emplace(type, cache->query_by_range(start_ts, end_ts));
    }

    return result;
}

QueryResult MultiResourceTimelineCache::query(ResourceType type, Timestamp ts) const
{
    QueryResult result;
    result.type = type;
    result.start_ts = start_ts;
    result.end_ts = end_ts;
    result.use_shm = false;
    result.shm_handle_ptr = nullptr;
    // 获取对应资源类型的缓存
    TimelineCache* cache = nullptr;
    {
        std::shared_lock lock(resource_mutexes_lock);
        auto it = resource_caches.find(type);
        if (it != resource_caches.end()) {
            cache = it->second.get();
        }
    }
    if (!cache) {
        // 没有缓存，直接返回空结果
        return result;
    }
    // 加读锁，安全访问 TimelineCache
    std::shared_lock lock(resource_mutexes.at(type));
    // 查询缓存
    std::vector<UnifiedDataPacket*> packets = cache->query_range(start_ts, end_ts);
    for (UnifiedDataPacket* pkt : packets) {
        if (!pkt) continue;
        // 更新起止时间
        if (pkt->timestamp < result.start_ts) result.start_ts = pkt->timestamp;
        if (pkt->timestamp > result.end_ts) result.end_ts = pkt->timestamp;
        // 处理数据
        if (std::holds_alternative<SharedMemoryHandle>(pkt->data_ptr)) {
            // 使用共享内存
            result.use_shm = true;
            // 注意：QueryResult 当前只能保存一个 shm_handle_ptr，如果需要多个，可改为 vector
            result.shm_handle_ptr = &std::get<SharedMemoryHandle>(pkt->data_ptr);
        } else if (std::holds_alternative<void*>(pkt->data_ptr)) {
            // 普通内存，直接拷贝
            void* ptr = std::get<void*>(pkt->data_ptr);
            if (ptr && pkt->data_size > 0) {
                uint8_t* byte_ptr = static_cast<uint8_t*>(ptr);
                result.data.insert(result.data.end(), byte_ptr, byte_ptr + pkt->data_size);
            }
        }
    }
    // 检查是否有缓存未覆盖的部分，必要时从持久化加载
    // if (!is_range_completely_covered_by_cache(type, start_ts, end_ts) && persistence_engine) {
    //     QueryResult persisted = persistence_engine->query_by_range(type, start_ts, end_ts);
    //     result = merge_query_results(result, persisted);
    // }
    return result;
    // QueryResult result;

    // // 1. 查找资源类型的缓存
    // auto it = resource_caches.find(type);
    // if (it == resource_caches.end()) {
    //     return result; // 返回空结果
    // }

    // // 2. 加锁保护
    // std::shared_lock<std::shared_mutex> lock(resource_mutexes.at(type));

    // // 3. 查询缓存数据
    // TimelineCache *cache = it->second.get();
    // if (cache) {
    //     result.resource_data.emplace(type, cache->query_by_range(start_ts, end_ts));
    // }

    // return result;
}

MultiResourceQueryResult MultiResourceTimelineCache::query_all_resources_by_range(Timestamp start_ts, Timestamp end_ts) const {

}

// 立即持久化
void MultiResourceTimelineCache::trigger_immediate_persistence(ResourceType type) {

}

// 检查点相关代码
// 插入检查点
void MultiResourceTimelineCache::insert_checkpoint(Timestamp timestamp) {
    checkpoint_manager->insert_checkpoint(timestamp);
}

// 删除指定时间戳之前的检查点
void MultiResourceTimelineCache::remove_checkpoints_before(Timestamp timestamp) {
    checkpoint_manager->remove_checkpoints_before(timestamp);
}

// 查询指定时间戳之前的检查点
std::vector<Timestamp> MultiResourceTimelineCache::query_checkpoints_before(Timestamp timestamp) {
    return checkpoint_manager->query_checkpoints_before(timestamp);
}

// 查询指定时间范围内的检查点
std::vector<Timestamp> MultiResourceTimelineCache::query_checkpoints_in_range(Timestamp start, Timestamp end) {
    return checkpoint_manager->query_checkpoints_range(start, end);
}

// 找最接近指定时间戳的检查点
std::optional<Timestamp> MultiResourceTimelineCache::find_nearest_checkpoint(Timestamp timestamp) {
    return checkpoint_manager->find_nearest_checkpoint(timestamp);
}

// 获取最新检查点
std::optional<Timestamp> MultiResourceTimelineCache::get_latest_checkpoint() {
    return checkpoint_manager->get_latest_checkpoint();
}

// 获取最早检查点
std::optional<Timestamp> MultiResourceTimelineCache::get_earliest_checkpoint() {
    return checkpoint_manager->get_earliest_checkpoint();
}

// 检查是否存在指定检查点
bool MultiResourceTimelineCache::has_checkpoint(Timestamp timestamp) {
    return checkpoint_manager->has_checkpoint(timestamp);
}

// 获取检查点总数
size_t MultiResourceTimelineCache::get_checkpoint_count() {
    return checkpoint_manager->get_checkpoint_count();
}

// 清空所有检查点
void MultiResourceTimelineCache::clear_all_checkpoints() {
    return checkpoint_manager->clear_all_checkpoint();
}

// 批量插入检查点
void MultiResourceTimelineCache::batch_insert_checkpoints(const std::vector<Timestamp>& timestamps) {
    return checkpoint_manager->insert_batch_checkpoint(timestamps);
}

// 共享内存相关接口
SharedMemoryHandle MultiResourceTimelineCache::allocate_shared_memory(size_t size, ResourceType type) {

}

bool MultiResourceTimelineCache::deallocate_shared_memory(const SharedMemoryHandle& handle) {

}

bool MultiResourceTimelineCache::is_shared_memory_unreferenced(const SharedMemoryHandle& handle) {

}

// 获取共享内存引用计数
int MultiResourceTimelineCache::get_shared_memory_reference_count(const SharedMemoryHandle& handle) {

}

// 条件释放共享内存（仅当引用计数为0时释放）
bool MultiResourceTimelineCache::deallocate_shared_memory_if_unreferenced(const SharedMemoryHandle& handle) {

}

// 缓存管理接口
bool MultiResourceTimelineCache::enable_resource_cache(ResourceType type, bool enabled) {

}

CacheStatistics MultiResourceTimelineCache::get_cache_statistics() const {

}

TimelineCache* MultiResourceTimelineCache::get_or_create_cache(ResourceType type) {
    auto it = resource_caches.find(type);
    if (it == resource_caches.end()) {
        // 这边需要补充
        resource_caches[type] = std::make_unique<TimelineCache>();
        return resource_caches[type].get();
    }
    return it->second.get();
}