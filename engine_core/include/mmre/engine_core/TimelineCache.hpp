#pragma once

#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>
#include <string>
#include "mmre/common/Types.hpp"
#include "mmre/engine_core/UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Core Timeline Cache using a Red-Black Tree + Doubly Linked List hybrid structure.
 */
class TimelineCache {
public:
    TimelineCache() = default;
    ~TimelineCache() = default;

    /**
     * @brief Inserts a new data packet into the timeline.
     */
    void Insert(std::shared_ptr<UnifiedDataPacket> packet);

    /**
     * @brief Queries the latest state of a resource at a specific point in time.
     */
    std::shared_ptr<UnifiedDataPacket> QuerySnapshot(const std::string& resourceId, 
                                                     const std::string& subResourceId,
                                                     common::TimestampNs targetTime, 
                                                     common::TimestampNs fallbackWindowNs);

    /**
     * @brief Range query for historical data back-tracking.
     */
    std::vector<std::shared_ptr<UnifiedDataPacket>> QueryRange(const std::string& resourceId, 
                                                               const std::string& subResourceId,
                                                               common::TimestampNs startTime, 
                                                               common::TimestampNs endTime);

    /**
     * @brief [架构优化说明] 缓存逐出机制。当内存达到水位线时，由 PersistenceEngine 调用，释放冷数据。
     */
    void EvictColdData(uint64_t targetFreeBytes);

private:
    struct CacheSegment {
        std::shared_mutex rwLock; 
        std::map<common::TimestampNs, std::shared_ptr<UnifiedDataPacket>> index;
        std::shared_ptr<UnifiedDataPacket> listHead;
        std::shared_ptr<UnifiedDataPacket> listTail;
    };

    std::string GenerateSegmentKey(const std::string& resId, const std::string& subResId);

    std::map<std::string, std::unique_ptr<CacheSegment>> segments_;
    std::shared_mutex segmentsLock_; 
};

} // namespace engine_core
} // namespace mmre
