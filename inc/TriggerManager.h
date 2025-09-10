#include "TimelineCache.h"
#include "MultiResourceTimelineCache.h"

#include <functional>

// 检查点数据结构
struct CheckpointData {
    Timestamp reference_timestamp;
    ResourceType original_data_type;
    std::string checkpoint_type;
    std::vector<uint8_t> state_snapshot;
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
        ResourceType resource_type;  // 改为使用 DataType
        std::chrono::milliseconds interval;
        std::function<bool(const UnifiedDataPacket&)> detection_func;  // 使用 UnifiedDataPacket
        double threshold_value;
        bool enabled;
    };

private:
    MultiResourceTimelineCache* cache;
    std::map<ResourceType, std::vector<TriggerConfig>> triggers;
    std::map<ResourceType, Timestamp> last_checkpoint_time;
    std::mutex trigger_mutex;

public:
    TriggerManager(MultiResourceTimelineCache* c) : cache(c) {}

    // 注册触发器
    void register_trigger(const TriggerConfig& config);

    // 检查是否需要创建检查点（数据输入时调用）
    void check_and_create_checkpoint(const UnifiedDataPacket& packet);

private:
    bool check_time_interval_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
        auto last_time_it = last_checkpoint_time.find(packet.type);
        if (last_time_it == last_checkpoint_time.end()) {
            last_checkpoint_time[packet.type] = packet.timestamp;
            return true;
        }

        auto elapsed = packet.timestamp - last_time_it->second;
        return elapsed >= config.interval.count();
    }

    bool check_event_detection_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config);

    bool check_threshold_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config);

    void create_checkpoint(const UnifiedDataPacket& reference_packet);

    std::vector<uint8_t> serialize_checkpoint_data(const CheckpointData& data);

    std::string datatype_to_string(ResourceType type);
};