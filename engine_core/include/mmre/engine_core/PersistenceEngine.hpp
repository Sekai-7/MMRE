#pragma once

#include <memory>
#include "mmre/common/Types.hpp"
#include "mmre/engine_core/TimelineCache.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Asynchronous persistence engine for migrating data from RAM to UFS.
 * 
 * // [架构优化说明]
 * // 由于 UFS (闪存) 写入具有长尾延迟，绝不能在主线程处理。
 * // 采用双缓冲队列和 Worker Pool 异步线程池进行抽帧、降采样（Downsampling）和磁盘 I/O。
 * // 确保冷热数据平滑交换，保障前台 Cache 的 10ms 实时查询响应不被打断。
 */
class PersistenceEngine {
public:
    explicit PersistenceEngine(std::shared_ptr<TimelineCache> cache);
    ~PersistenceEngine();

    /**
     * @brief Starts the background persistence daemon thread.
     */
    void Start();

    /**
     * @brief Forces an immediate sync of data up to a specified timestamp (e.g. Memory Threshold triggered).
     */
    void ForceSync(common::TimestampNs upToTime);

private:
    std::shared_ptr<TimelineCache> cache_;
    // TODO: Thread management, queues, and strategy references (Strategy Pattern)
};

} // namespace engine_core
} // namespace mmre
