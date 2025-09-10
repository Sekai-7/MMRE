#include "MultiResourceCheckpointManager.h"

void MultiResourceCheckpointManager::insert_checkpoint(Timestamp timestamp) {
    std::unique_lock<std::shared_mutex> lock(checkpoints_mutex);
    checkpoints.insert(checkpoints.end(), timestamp);
}

void MultiResourceCheckpointManager::insert_batch_checkpoint(const std::vector<Timestamp>& timestamps) {
    std::unique_lock<std::shared_mutex> lock(checkpoints_mutex);

    for (const auto& timestamp : timestamps) {
        checkpoints.insert(checkpoints.end(), timestamp);
    }

    return;
}

size_t MultiResourceCheckpointManager::remove_checkpoints_before(Timestamp timestamp) {
    std::unique_lock<std::shared_mutex> lock(checkpoints_mutex);

    auto it = checkpoints.lower_bound(timestamp);

    size_t removed_count = std::distance(checkpoints.begin(), it);
    
    checkpoints.erase(checkpoints.begin(), it);

    return removed_count;
}

std::vector<Timestamp> MultiResourceCheckpointManager::query_checkpoints_before(Timestamp timestamp) const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    return std::vector<Timestamp>(checkpoints.begin(), checkpoints.lower_bound(timestamp));
}

std::vector<Timestamp> MultiResourceCheckpointManager::query_checkpoints_range(Timestamp start, Timestamp end) const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    return std::vector<Timestamp>(checkpoints.lower_bound(start), checkpoints.upper_bound(end));
}

std::optional<Timestamp> MultiResourceCheckpointManager::find_nearest_checkpoint(Timestamp timestamp) const {
    if (checkpoints.empty())
        return std::nullopt;
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

std::optional<Timestamp> MultiResourceCheckpointManager::get_latest_checkpoint() const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    if (checkpoints.empty())
        return std::nullopt;
    
    return *checkpoints.rbegin();
}

std::optional<Timestamp> MultiResourceCheckpointManager::get_earliest_checkpoint() const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    if (checkpoints.empty())
        return std::nullopt;
    
    return *checkpoints.begin();
}

size_t MultiResourceCheckpointManager::get_checkpoint_count() const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    return checkpoints.size();
}

void MultiResourceCheckpointManager::clear_all_checkpoint() {
    std::unique_lock<std::shared_mutex> lock(checkpoints_mutex);
    checkpoints.clear();
}

bool MultiResourceCheckpointManager::has_checkpoint(Timestamp timestamp) const {
    std::shared_lock<std::shared_mutex> lock(checkpoints_mutex);
    return checkpoints.find(timestamp) != checkpoints.end();
}