#pragma once

#include <string>
#include <vector>
#include "mmre/common/Types.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Represents a semantic anchor in the timeline.
 */
struct Checkpoint {
    common::TimestampNs timestamp;
    std::string triggerSource; ///< Which Trigger or Agent created this
    std::string description;   ///< Semantic description (e.g. "Driver Fatigue Detected")
};

/**
 * @brief Manages crucial event checkpoints for synchronized persistence and queries.
 * 
 * // [架构优化说明]
 * // 将 Checkpoint 管理独立出来。当系统触发检查点时，引擎会强制锁定该时间窗内的数据，
 * // 避免被普通的 LRU 或内存水位清理机制驱逐，确保 AI 推理所需的“因果链”数据完整落盘。
 */
class CheckpointManager {
public:
    void AddCheckpoint(const Checkpoint& cp);
    std::vector<Checkpoint> GetCheckpoints(common::TimestampNs start, common::TimestampNs end);
    bool IsWithinCriticalWindow(common::TimestampNs time);

private:
    std::vector<Checkpoint> checkpoints_;
    // Time window radius (e.g. +/- 5 seconds around a checkpoint)
    common::TimestampNs criticalWindowRadiusNs_{5000000000}; 
};

} // namespace engine_core
} // namespace mmre
