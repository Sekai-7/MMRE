#include "DataIngestionManager.h"

bool DataIngestionManager::ingest_data(UnifiedDataPacket&& packet) {
    if (!validate_packet(packet)) {
        // output error message
        return false;
    }

    return cache->insert(std::move(packet));
}

BatchIngestionResult DataIngestionManager::ingest_batch_data(std::vector<UnifiedDataPacket>&& packets) {
    BatchIngestionResult batch_ingestion_result;
    batch_ingestion_result.total_packets = packets.size();
    batch_ingestion_result.failed_packets = 0;
    batch_ingestion_result.fail_list.reserve(batch_ingestion_result.total_packets);

    if (packets.empty())
        return batch_ingestion_result;

    for (auto&& data_packet : packets) {
        if (validate_packet(data_packet) == false) {
            ++batch_ingestion_result.failed_packets;
            batch_ingestion_result.fail_list.emplace_back(std::move(data_packet));
        } else {
            cache->insert(std::move(data_packet));
        }
    }

    return batch_ingestion_result;
}


bool DataIngestionManager::validate_packet(const UnifiedDataPacket& packet) {
    if (packet.data_size <= 0)
        return false;

    struct Visitor {
        bool operator() (void* ptr) {
            if (ptr == nullptr)
                return false;
            return true;
        }

        bool operator() (SharedMemoryHandle handle) {
            if (handle.is_valid() == false || handle.get_size() == 0)
                return false;
            return true;
        }
    };

    if (std::visit(Visitor{}, packet.data_ptr) == false)
        return false;

    switch (packet.type) {
    case ResourceType::CAMERA:
        return validate_camera_shared_memory(packet);
    case ResourceType::AUDIO:
        return validate_audio_shared_memory(packet);
    case ResourceType::GPS:
        return validate_gps_shared_memory(packet);
    case ResourceType::IMU:
        return validate_imu_shared_memory(packet);
    case ResourceType::VEHICLE_SIGNAL:
        return validate_vehicle_signal_shared_memory(packet);
    default:
        return false;
    }
}

void DataIngestionManager::update_ingestion_stats(ResourceType rt) {
    ++ingestion_stats[rt];
    return;
}

IngestionStats DataIngestionManager::get_ingestion_stats() const {
    std::lock_guard<std::mutex> lg(stats_mutex);
    IngestionStats is;

    for (const auto& p : ingestion_stats) {
        is.packets_per_ResourceType[p.first] = p.second;
    }

    is.last_update_time = std::time(nullptr);
    return is;
}

void DataIngestionManager::setup_default_triggers() {
    // wait complete
}
