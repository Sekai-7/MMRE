#include "TimelineCacheBenchmark.h"

void TimelineCacheBaseLine::insert(long long timestamp, DataType type, void* data_ptr, size_t data_size) {
    UnifiedDataPacket packet{timestamp, type, data_ptr, data_size};
    data_map[timestamp] = packet;
    data_deque.push_back(&data_map[timestamp]);
}

UnifiedDataPacket* TimelineCacheBaseLine::find(long long timestamp) {
    // 使用 map 进行查找 (O(log N))
    auto it = data_map.find(timestamp);
    if (it != data_map.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<UnifiedDataPacket> TimelineCacheBaseLine::query_range(long long start_ts, long long end_ts) {
    // 利用 map 的有序性进行范围查询 (O(log N + K))
    std::vector<UnifiedDataPacket> results;
    auto it_start = data_map.lower_bound(start_ts);
    auto it_end = data_map.upper_bound(end_ts);
    for (auto it = it_start; it != it_end; ++it) {
        results.push_back(it->second);
    }
    return results;
}

UnifiedDataPacket TimelineCacheBaseLine::evict_oldest() {
    if (data_deque.empty()) {
        throw std::runtime_error("Cache is empty");
    }
    UnifiedDataPacket* oldest_packet = data_deque.front();
    data_deque.pop_front();
    UnifiedDataPacket packet_to_return = *oldest_packet; 
    // 2. 从 map 中删除对应的数据 (O(log N))。这里需要注意的是，由于deque中存储的是指针，因此删除map中的元素后，deque中的指针会失效，所以需要先拷贝数据再删除
    data_map.erase(oldest_packet->timestamp);
    return packet_to_return;
}
void TimelineCacheBaseLine::insertCameraData(long long timestamp, const char* image_data, size_t image_size) {
    void* shm_ptr = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        // ... error handling
    }
    memcpy(shm_ptr, image_data, image_size);
    insert(timestamp, DataType::CAMERA, shm_ptr, image_size);
}

void TimelineCacheBaseLine::insertAudioData(long long timestamp, const char* audio_data, size_t audio_size) {
    void* shm_ptr = mmap(NULL, audio_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        // ... error handling
    }
    memcpy(shm_ptr, audio_data, audio_size);
    insert(timestamp, DataType::AUDIO, shm_ptr, audio_size);
}