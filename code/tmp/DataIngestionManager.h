#ifndef DATAINGESTIONMANAGER_H
#define DATAINGESTIONMANAGER_H

#include "common.h"

#include <map>
#include <string>
#include <vector>

struct MultiResourceTimelineCache;
struct TriggerManager;

struct BatchIngestionResult {
    size_t totalPackets;
    size_t failedPackets;
    std::vector<UnifiedDataPacket> failList;
};


struct IngestionStats {
    std::map<ResourceType, uint64_t> packetsPerResourceType;
    Timestamp lastUpdateTime;
};

class DataIngestionManager {
private:
    MultiResourceTimelineCache* cache;
    std::unique_ptr<TriggerManager> triggerManager;
    std::map<ResourceType, std::atomic<uint64_t>> ingestionStats;
    mutable std::mutex statsMutex;
    
    // bool useSharedMemory;
    std::map<ResourceType, size_t> defaultSharedMemorySizes;  // 每种数据类型的默认共享内存大小

public:
    DataIngestionManager(MultiResourceTimelineCache* c);

    ~DataIngestionManager();

    bool ingestData(UnifiedDataPacket&& packet);

    BatchIngestionResult ingestBatchData(std::vector<UnifiedDataPacket>&&);

    TriggerManager* getTriggerManager();

    IngestionStats getIngestionStats() const;

    void setupDefaultTriggers();

    // bool ingestDataWithSharedMemory(const UnifiedDataPacket& packet, size_t sharedMemorySize);
    // BatchIngestionResult ingestBatchData(const std::vector<UnifiedDataPacket>& packets);
    // bool ingestDataZeroCopy(UnifiedDataPacket& packet, const SharedMemoryHandle& handle);
    // void setSharedMemoryEnabled(bool enabled);
    // bool isSharedMemoryEnabled() const;
    // void setDefaultSharedMemorySize(ResourceType type, size_t size);
    // size_t getDefaultSharedMemorySize(ResourceType type) const;

private:
    bool validatePacket(const UnifiedDataPacket&);
    bool validateCameraSharedMemory(const UnifiedDataPacket&);
    bool validateAudioSharedMemory(const UnifiedDataPacket&);
    bool validateGpsSharedMemory(const UnifiedDataPacket&);
    bool validateImuSharedMemory(const UnifiedDataPacket&);
    bool validateVehicleSignalSharedMemory(const UnifiedDataPacket&);

    void updateIngestionStats(ResourceType);
};

#endif