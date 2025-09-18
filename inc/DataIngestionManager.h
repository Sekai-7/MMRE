#include "TimelineCache.h"
#include "MultiResourceTimelineCache.h"
#include "TriggerManager.h"

#include <map>
#include <string>
#include <vector>

struct BatchIngestionResult {
    size_t total_packets;
    size_t failed_packets;
    std::vector<UnifiedDataPacket&&> fail_list;
};


struct IngestionStats {
    std::map<ResourceType, uint64_t> packets_per_ResourceType;
    Timestamp last_update_time;
};

class DataIngestionManager {
private:
    MultiResourceTimelineCache* cache;
    std::unique_ptr<TriggerManager> trigger_manager;
    std::map<ResourceType, std::atomic<uint64_t>> ingestion_stats;
    mutable std::mutex stats_mutex;
    
    // bool use_shared_memory;
    std::map<ResourceType, size_t> default_shared_memory_sizes;  // 每种数据类型的默认共享内存大小

public:
    DataIngestionManager(MultiResourceTimelineCache* c);

    bool ingest_data(UnifiedDataPacket&& packet);

    BatchIngestionResult ingest_batch_data(std::vector<UnifiedDataPacket>&&);

    TriggerManager* get_trigger_manager();

    IngestionStats get_ingestion_stats() const;

    void setup_default_triggers();


    BatchIngestionResult ingest_batch_data(std::vector<UnifiedDataPacket>&&);

    TriggerManager* get_trigger_manager();

    IngestionStats get_ingestion_stats() const;

    void setup_default_triggers();

    // bool ingest_data_with_shared_memory(const UnifiedDataPacket& packet, size_t shared_memory_size);
    // BatchIngestionResult ingest_batch_data(const std::vector<UnifiedDataPacket>& packets);
    // bool ingest_data_zero_copy(UnifiedDataPacket& packet, const SharedMemoryHandle& handle);
    // void set_shared_memory_enabled(bool enabled);
    // bool is_shared_memory_enabled() const;
    // void set_default_shared_memory_size(ResourceType type, size_t size);
    // size_t get_default_shared_memory_size(ResourceType type) const;

private:
    bool validate_packet(const UnifiedDataPacket&);
    bool validate_camera_shared_memory(const UnifiedDataPacket&);
    bool validate_audio_shared_memory(const UnifiedDataPacket&);
    bool validate_gps_shared_memory(const UnifiedDataPacket&);
    bool validate_imu_shared_memory(const UnifiedDataPacket&);
    bool validate_vehicle_signal_shared_memory(const UnifiedDataPacket&);

    void update_ingestion_stats(ResourceType);
};