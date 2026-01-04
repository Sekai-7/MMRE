#include "TriggerManager.h"
#include "MultiResourceTimelineCache.h"

void TriggerManager::registerTrigger(const TriggerConfig& config) {
    std::lock_guard<std::mutex> lock(triggerMutex);
    triggers[config.resourceType].push_back(config);
    return;
}

void TriggerManager::checkAndCreateCheckpoint(const UnifiedDataPacket& packet) {
    std::lock_guard<std::mutex> lock(triggerMutex);
    
    auto triggerIt = triggers.find(packet.type);
    if (triggerIt == triggers.end()) {
        return;
    }

    for (const auto& trigger : triggerIt->second) {
        if (!trigger.enabled) continue;

        bool shouldCreateCheckpoint = false;

        switch (trigger.type) {
            case TriggerType::TIME_INTERVAL:
                shouldCreateCheckpoint = checkTimeIntervalTrigger(packet, trigger);
                break;

            case TriggerType::EVENT_DETECTION:
                shouldCreateCheckpoint = checkEventDetectionTrigger(packet, trigger);
                break;

            case TriggerType::DATA_THRESHOLD:
                shouldCreateCheckpoint = checkThresholdTrigger(packet, trigger);
                break;

            case TriggerType::MANUAL:
                break;
        }

        if (shouldCreateCheckpoint) {
            createCheckpoint(packet);
            break;
        }
    }
}

bool TriggerManager::checkTimeIntervalTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
    auto lastTimeIt = lastCheckpointTime.find(packet.type);
    if (lastTimeIt == lastCheckpointTime.end()) {
        lastCheckpointTime[packet.type] = packet.timestamp;
        return true;
    }

    auto elapsed = packet.timestamp - lastTimeIt->second;
    return elapsed >= config.interval.count();
}

bool TriggerManager::checkEventDetectionTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
    if (config.detectionFunc) {
        return config.detectionFunc(packet);
    }
    return false;
}

// 需要修改
bool TriggerManager::checkThresholdTrigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
    switch (packet.type) {
        case ResourceType::GPS: {
            // 假设GPS数据格式包含速度信息
            break;
        }
        case ResourceType::IMU: {
            // 假设IMU数据格式包含加速度信息
            break;
        }
        case ResourceType::VEHICLE_SIGNAL: {
            // 假设车辆信号数据格式
            break;
        }
        default:
            return 0.0;
    }
    return false;
}

void TriggerManager::createCheckpoint(const UnifiedDataPacket& referencePacket) {
        // 直接插入时间戳作为检查点
    // bool success = cache->insertCheckpoint(referencePacket.timestamp);
    cache->insertCheckpoint(referencePacket.timestamp);
    
    // if (success) {
    //     lastCheckpointTime[referencePacket.type] = referencePacket.timestamp;
    //     // log_info("Created checkpoint at timestamp " + std::to_string(referencePacket.timestamp));
    // } else {
    //     // log_error("Failed to create checkpoint at timestamp " + std::to_string(referencePacket.timestamp));
    // }
}

std::string TriggerManager::datatypeToString(ResourceType type) {
    std::string ret;
    switch (type)
    {
    case ResourceType::CAMERA:
        ret = "Camera";
        break;
    case ResourceType::AUDIO:
        ret = "AUDIO";
        break;
    case ResourceType::GPS:
        ret = "GPS";
        break;
    case ResourceType::IMU:
        ret = "IMU";
        break;
    case ResourceType::VEHICLE_SIGNAL:
        ret = "VEHICLE_SIGNAL";
        break;
    default:
        break;
    }
    return ret;
}

std::vector<uint8_t> TriggerManager::serializeCheckpointData(const CheckpointData& data) {
    return {};
}