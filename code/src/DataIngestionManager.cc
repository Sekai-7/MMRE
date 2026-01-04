#include "DataIngestionManager.h"
#include "MultiResourceTimelineCache.h"
#include "TriggerManager.h"

DataIngestionManager::~DataIngestionManager() = default;

DataIngestionManager::DataIngestionManager(MultiResourceTimelineCache* c):cache(c), triggerManager(std::make_unique<TriggerManager>(c)) {}

bool DataIngestionManager::ingestData(UnifiedDataPacket&& packet) {
    if (!validatePacket(packet)) {
        // output error message
        return false;
    }

    return cache->insert(std::move(packet));
}

BatchIngestionResult DataIngestionManager::ingestBatchData(std::vector<UnifiedDataPacket>&& packets) {
    BatchIngestionResult batchIngestionResult;
    batchIngestionResult.totalPackets = packets.size();
    batchIngestionResult.failedPackets = 0;
    batchIngestionResult.failList.reserve(batchIngestionResult.totalPackets);

    if (packets.empty())
        return batchIngestionResult;

    for (auto&& dataPacket : packets) {
        if (validatePacket(dataPacket) == false) {
            ++batchIngestionResult.failedPackets;
            batchIngestionResult.failList.emplace_back(std::move(dataPacket));
        } else {
            cache->insert(std::move(dataPacket));
        }
    }

    return batchIngestionResult;
}


bool DataIngestionManager::validatePacket(const UnifiedDataPacket& packet) {
    if (packet.dataSize <= 0)
        return false;

    // struct Visitor {
    //     bool operator() (void* ptr) {
    //         if (ptr == nullptr)
    //             return false;
    //         return true;
    //     }

    //     bool operator() (SharedMemoryHandle handle) {
    //         if (handle.isValid() == false || handle.getSize() == 0)
    //             return false;
    //         return true;
    //     }
    // };

    // if (std::visit(Visitor{}, packet.dataPtr) == false)
        // return false;

    if (packet.getPtr() == nullptr)
        return false;

    switch (packet.type) {
    case ResourceType::CAMERA:
        return validateCameraSharedMemory(packet);
    case ResourceType::AUDIO:
        return validateAudioSharedMemory(packet);
    case ResourceType::GPS:
        return validateGpsSharedMemory(packet);
    case ResourceType::IMU:
        return validateImuSharedMemory(packet);
    case ResourceType::VEHICLE_SIGNAL:
        return validateVehicleSignalSharedMemory(packet);
    default:
        return false;
    }
}

void DataIngestionManager::updateIngestionStats(ResourceType rt) {
    ++ingestionStats[rt];
    return;
}

IngestionStats DataIngestionManager::getIngestionStats() const {
    std::lock_guard<std::mutex> lg(statsMutex);
    IngestionStats is;

    for (const auto& p : ingestionStats) {
        is.packetsPerResourceType[p.first] = p.second;
    }

    is.lastUpdateTime = std::time(nullptr);
    return is;
}

void DataIngestionManager::setupDefaultTriggers() {
    // wait complete
}

bool DataIngestionManager::validateCameraSharedMemory(const UnifiedDataPacket&) {
    return true;
}

bool DataIngestionManager::validateAudioSharedMemory(const UnifiedDataPacket&) {
    return true;
}
bool DataIngestionManager::validateGpsSharedMemory(const UnifiedDataPacket&) {
    return true;
}
bool DataIngestionManager::validateImuSharedMemory(const UnifiedDataPacket&) {
    return true;
}
bool DataIngestionManager::validateVehicleSignalSharedMemory(const UnifiedDataPacket&) {
    return true;
}