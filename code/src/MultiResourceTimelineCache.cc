#include "MultiResourceTimelineCache.h"
#include "CacheNodePool.h"

bool MultiResourceTimelineCache::insert(UnifiedDataPacket&& data) {
    TimelineCache* cache = getOrCreateCache(data.type);
    if (cache == nullptr)
        return false;
    cache->insert(std::move(data));
    return true;
}

QueryResult MultiResourceTimelineCache::queryByRange(
    ResourceType type, Timestamp startTs, Timestamp endTs) const 
{
    QueryResult result;
    result.type = type;
    result.startTs = startTs;
    result.endTs = endTs;

    auto it = resourceCaches.find(type);
    if (it == resourceCaches.end() || !it->second) {
        return result;
    }

    auto* cache = it->second.get();
    auto packets = cache->queryByRange(startTs, endTs);

    for (auto* pkt : packets) {
        if (!pkt) continue;

        if (std::holds_alternative<SharedMemoryHandle>(pkt->dataPtr)) {
            // 共享内存模式
            auto& handle = std::get<SharedMemoryHandle>(pkt->dataPtr);
            if (handle.isValid()) {
                result.shmHandles.push_back(&handle);
                // result.useShm = true;
            }
        } else if (std::holds_alternative<void*>(pkt->dataPtr)) {
            // 普通内存模式
            void* ptr = std::get<void*>(pkt->dataPtr);
            if (ptr && pkt->dataSize > 0) {
                uint8_t* raw = static_cast<uint8_t*>(ptr);
                result.data.insert(result.data.end(), raw, raw + pkt->dataSize);
            }
        }
    }

    return result;
}

// 查询数据
QueryResult MultiResourceTimelineCache::query(ResourceType type, Timestamp ts) const {
    QueryResult result;
    result.type = type;
    result.startTs = ts;
    result.endTs = ts;

    auto it = resourceCaches.find(type);
    if (it == resourceCaches.end() || !it->second) {
        return result;
    }

    auto* cache = it->second.get();
    auto packet = cache->query(ts);

    if (!packet) {
        result.startTs = -1;
        result.endTs = -1;
        return result;
    }

    if (std::holds_alternative<SharedMemoryHandle>(packet->dataPtr)) {
        // 共享内存模式
        auto& handle = std::get<SharedMemoryHandle>(packet->dataPtr);
        if (handle.isValid()) {
            result.shmHandles.push_back(&handle);
            // result.useShm = true;
        }
    } else if (std::holds_alternative<void*>(packet->dataPtr)) {
        // 普通内存模式
        void* ptr = std::get<void*>(packet->dataPtr);
        if (ptr && packet->dataSize > 0) {
            uint8_t* raw = static_cast<uint8_t*>(ptr);
            result.data.insert(result.data.end(), raw, raw + packet->dataSize);
        }
    }

    return result;
}

MultiResourceQueryResult MultiResourceTimelineCache::queryAllResourcesByRange(Timestamp startTs, Timestamp endTs) const {
    return {};
}

// 立即持久化
void MultiResourceTimelineCache::triggerImmediatePersistence(ResourceType type) {

}

// 检查点相关代码
// 插入检查点
void MultiResourceTimelineCache::insertCheckpoint(Timestamp timestamp) {
    checkpointManager->insertCheckpoint(timestamp);
}

// 删除指定时间戳之前的检查点
void MultiResourceTimelineCache::removeCheckpointsBefore(Timestamp timestamp) {
    checkpointManager->removeCheckpointsBefore(timestamp);
}

// 查询指定时间戳之前的检查点
std::vector<Timestamp> MultiResourceTimelineCache::queryCheckpointsBefore(Timestamp timestamp) {
    return checkpointManager->queryCheckpointsBefore(timestamp);
}

// 查询指定时间范围内的检查点
std::vector<Timestamp> MultiResourceTimelineCache::queryCheckpointsInRange(Timestamp start, Timestamp end) {
    return checkpointManager->queryCheckpointsRange(start, end);
}

// 找最接近指定时间戳的检查点
std::optional<Timestamp> MultiResourceTimelineCache::findNearestCheckpoint(Timestamp timestamp) {
    return checkpointManager->findNearestCheckpoint(timestamp);
}

// 获取最新检查点
std::optional<Timestamp> MultiResourceTimelineCache::getLatestCheckpoint() {
    return checkpointManager->getLatestCheckpoint();
}

// 获取最早检查点
std::optional<Timestamp> MultiResourceTimelineCache::getEarliestCheckpoint() {
    return checkpointManager->getEarliestCheckpoint();
}

// 检查是否存在指定检查点
bool MultiResourceTimelineCache::hasCheckpoint(Timestamp timestamp) {
    return checkpointManager->hasCheckpoint(timestamp);
}

// 获取检查点总数
size_t MultiResourceTimelineCache::getCheckpointCount() {
    return checkpointManager->getCheckpointCount();
}

// 清空所有检查点
void MultiResourceTimelineCache::clearAllCheckpoints() {
    return checkpointManager->clearAllCheckpoint();
}

// 批量插入检查点
void MultiResourceTimelineCache::batchInsertCheckpoints(const std::vector<Timestamp>& timestamps) {
    return checkpointManager->insertBatchCheckpoint(timestamps);
}

// 共享内存相关接口
SharedMemoryHandle MultiResourceTimelineCache::allocateSharedMemory(size_t size, ResourceType type) {
    return {};
}

bool MultiResourceTimelineCache::deallocateSharedMemory(const SharedMemoryHandle& handle) {
    return true;
}

bool MultiResourceTimelineCache::isSharedMemoryUnreferenced(const SharedMemoryHandle& handle) {
    return true;
}

// 获取共享内存引用计数
int MultiResourceTimelineCache::getSharedMemoryReferenceCount(const SharedMemoryHandle& handle) {
    return 0;
}

// 条件释放共享内存（仅当引用计数为0时释放）
bool MultiResourceTimelineCache::deallocateSharedMemoryIfUnreferenced(const SharedMemoryHandle& handle) {
    return true;
}

// 缓存管理接口
bool MultiResourceTimelineCache::enableResourceCache(ResourceType type, bool enabled) {
    return true;
}

TimelineCache* MultiResourceTimelineCache::getOrCreateCache(ResourceType type) {
    auto it = resourceCaches.find(type);
    if (it == resourceCaches.end()) {
        // 这边需要补充
        resourceCaches[type] = std::make_unique<TimelineCache>();
        return resourceCaches[type].get();
    }
    return it->second.get();
}