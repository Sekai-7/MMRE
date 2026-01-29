#include "MultiResourceCheckpointManager.h"

#include <cmath>

void MultiResourceCheckpointManager::insertCheckpoint(Timestamp timestamp) {
    std::unique_lock<std::mutex> lock(checkpointsMutex);
    checkpoints.insert(checkpoints.end(), timestamp);
}

void MultiResourceCheckpointManager::insertBatchCheckpoint(const std::vector<Timestamp>& timestamps) {
    std::unique_lock<std::mutex> lock(checkpointsMutex);

    for (const auto& timestamp : timestamps) {
        checkpoints.insert(checkpoints.end(), timestamp);
    }

    return;
}

size_t MultiResourceCheckpointManager::removeCheckpointsBefore(Timestamp timestamp) {
    std::unique_lock<std::mutex> lock(checkpointsMutex);

    auto it = checkpoints.lower_bound(timestamp);
    
    size_t oldSize = checkpoints.size();
    checkpoints.erase(checkpoints.begin(), it);

    return oldSize - checkpoints.size();
}

std::vector<Timestamp> MultiResourceCheckpointManager::queryCheckpointsBefore(Timestamp timestamp) const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    return std::vector<Timestamp>(checkpoints.begin(), checkpoints.lower_bound(timestamp));
}

std::vector<Timestamp> MultiResourceCheckpointManager::queryCheckpointsRange(Timestamp start, Timestamp end) const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    if (start > end) {
        return {};
    }
    return std::vector<Timestamp>(checkpoints.lower_bound(start), checkpoints.upper_bound(end));
}

Timestamp MultiResourceCheckpointManager::findNearestCheckpoint(Timestamp timestamp) const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    if (checkpoints.empty())
        return -1;
    auto it = checkpoints.lower_bound(timestamp);

    if (it == checkpoints.begin()) {
        return *it;
    } else if (it == checkpoints.end()) {
        return *(std::prev(it));
    } else {
        auto prev = std::prev(it);
        if (std::abs(*prev - timestamp) <= std::abs(*it - timestamp)) {
            return *prev;
        } else {
            return *it;
        }
    }
}

Timestamp MultiResourceCheckpointManager::getLatestCheckpoint() const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    if (checkpoints.empty())
        return -1;
    
    return *checkpoints.rbegin();
}

Timestamp MultiResourceCheckpointManager::getEarliestCheckpoint() const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    if (checkpoints.empty())
        return -1;
    
    return *checkpoints.begin();
}

size_t MultiResourceCheckpointManager::getCheckpointCount() const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    return checkpoints.size();
}

void MultiResourceCheckpointManager::clearAllCheckpoints() {
    std::unique_lock<std::mutex> lock(checkpointsMutex);
    checkpoints.clear();
}

bool MultiResourceCheckpointManager::hasCheckpoint(Timestamp timestamp) const {
    std::lock_guard<std::mutex> lock(checkpointsMutex);
    return checkpoints.find(timestamp) != checkpoints.end();
}