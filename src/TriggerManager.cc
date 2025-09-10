#include "TriggerManager.h"

void TriggerManager::register_trigger(const TriggerConfig& config) {
    std::lock_guard<std::mutex> lock(trigger_mutex);
    triggers[config.resource_type].push_back(config);
    return;
}

void TriggerManager::check_and_create_checkpoint(const UnifiedDataPacket& packet) {
    std::lock_guard<std::mutex> lock(trigger_mutex);
    
    auto trigger_it = triggers.find(packet.type);
    if (trigger_it == triggers.end()) {
        return;
    }

    for (const auto& trigger : trigger_it->second) {
        if (!trigger.enabled) continue;

        bool should_create_checkpoint = false;

        switch (trigger.type) {
            case TriggerType::TIME_INTERVAL:
                should_create_checkpoint = check_time_interval_trigger(packet, trigger);
                break;

            case TriggerType::EVENT_DETECTION:
                should_create_checkpoint = check_event_detection_trigger(packet, trigger);
                break;

            case TriggerType::DATA_THRESHOLD:
                should_create_checkpoint = check_threshold_trigger(packet, trigger);
                break;

            case TriggerType::MANUAL:
                break;
        }

        if (should_create_checkpoint) {
            create_checkpoint(packet);
            break;
        }
    }
}

bool TriggerManager::check_time_interval_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
    auto last_time_it = last_checkpoint_time.find(packet.type);
    if (last_time_it == last_checkpoint_time.end()) {
        last_checkpoint_time[packet.type] = packet.timestamp;
        return true;
    }

    auto elapsed = packet.timestamp - last_time_it->second;
    return elapsed >= config.interval.count();
}

bool TriggerManager::check_event_detection_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
    if (config.detection_func) {
        return config.detection_func(packet);
    }
    return false;
}

// 需要修改
bool TriggerManager::check_threshold_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
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

void TriggerManager::create_checkpoint(const UnifiedDataPacket& reference_packet) {
        // 直接插入时间戳作为检查点
    bool success = cache->insert_checkpoint(reference_packet.timestamp);
    
    if (success) {
        last_checkpoint_time[reference_packet.type] = reference_packet.timestamp;
        // log_info("Created checkpoint at timestamp " + std::to_string(reference_packet.timestamp));
    } else {
        // log_error("Failed to create checkpoint at timestamp " + std::to_string(reference_packet.timestamp));
    }
}

std::string TriggerManager::datatype_to_string(ResourceType type) {
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

std::vector<uint8_t> TriggerManager::serialize_checkpoint_data(const CheckpointData& data) {

}