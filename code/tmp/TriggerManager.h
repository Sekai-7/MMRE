#ifndef TRIGGERMANAGER_H
#define TRIGGERMANAGER_H

#include "common.h"

#include <functional>
#include <string>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <map>

struct MultiResourceTimelineCache;

// 检查点数据结构
struct CheckpointData {
    Timestamp referenceTimestamp;
    ResourceType originalDataType;
    std::string checkpointType;
    std::vector<uint8_t> stateSnapshot;
};

class TriggerManager {
public:
    // 触发器类型枚举
    enum class TriggerType {
        TIME_INTERVAL,      // 时间间隔触发
        EVENT_DETECTION,    // 事件检测触发
        DATA_THRESHOLD,     // 数据阈值触发
        MANUAL             // 手动触发
    };

    // 触发器配置结构
    struct TriggerConfig {
        TriggerType type;
        ResourceType resourceType;  // 改为使用 DataType
        std::chrono::milliseconds interval;
        std::function<bool(const UnifiedDataPacket&)> detectionFunc;  // 使用 UnifiedDataPacket
        double thresholdValue;
        bool enabled;
    };

private:
    MultiResourceTimelineCache* cache;
    std::map<ResourceType, std::vector<TriggerConfig>> triggers;
    std::map<ResourceType, Timestamp> lastCheckpointTime;
    std::mutex triggerMutex;

public:
    TriggerManager(MultiResourceTimelineCache* c) : cache(c) {}

    // 注册触发器
    void registerTrigger(const TriggerConfig& config);

    // 检查是否需要创建检查点（数据输入时调用）
    void checkAndCreateCheckpoint(const UnifiedDataPacket& packet);

private:
    bool checkTimeIntervalTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config);

    bool checkEventDetectionTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config);

    bool checkThresholdTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config);

    void createCheckpoint(const UnifiedDataPacket& referencePacket);

    std::vector<uint8_t> serializeCheckpointData(const CheckpointData& data);

    std::string datatypeToString(ResourceType type);
};

#endif