#include "MultiResourceTimelineCache.h"

bool MultiResourceTimelineCache::insert(const UnifiedDataPacket& data) {
    TimelineCache* cache = get_or_create_cache(data.type);
    if (cache == nullptr)
        return false;
    cache->insert(data);
    return true;
}

// 查询数据
QueryResult MultiResourceTimelineCache::query_by_range(ResourceType type, Timestamp start_ts, Timestamp end_ts) const {

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