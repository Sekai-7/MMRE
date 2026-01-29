#ifndef MULTIRESOURCECHECKPOINTMANAGER_H
#define MULTIRESOURCECHECKPOINTMANAGER_H

#include "common.h"

#include <set>
#include <shared_mutex>
#include <vector>

class MultiResourceCheckpointManager {
private:
    std::set<Timestamp> checkpoints;
    mutable std::mutex checkpointsMutex;
public:
    MultiResourceCheckpointManager() = default; 
    ~MultiResourceCheckpointManager() = default;

    void insertCheckpoint(Timestamp);
    void insertBatchCheckpoint(const std::vector<Timestamp>&);

    size_t removeCheckpointsBefore(Timestamp);

    std::vector<Timestamp> queryCheckpointsBefore(Timestamp) const;
    std::vector<Timestamp> queryCheckpointsRange(Timestamp, Timestamp) const;

    Timestamp findNearestCheckpoint(Timestamp) const;
    Timestamp getLatestCheckpoint() const;
    Timestamp getEarliestCheckpoint() const;

    size_t getCheckpointCount() const;

    void clearAllCheckpoints();

    bool hasCheckpoint(Timestamp) const;
};

#endif