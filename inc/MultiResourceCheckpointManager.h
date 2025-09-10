#include "TimelineCache.h"

#include <set>
#include <shared_mutex>
#include <optional>

class MultiResourceCheckpointManager {
private:
    std::set<Timestamp> checkpoints;
    mutable std::shared_mutex checkpoints_mutex;
public:
    MultiResourceCheckpointManager() = default; 
    ~MultiResourceCheckpointManager() = default;

    void insert_checkpoint(Timestamp);
    void insert_batch_checkpoint(const std::vector<Timestamp>&);

    size_t remove_checkpoints_before(Timestamp);

    std::vector<Timestamp> query_checkpoints_before(Timestamp) const;
    std::vector<Timestamp> query_checkpoints_range(Timestamp, Timestamp) const;

    std::optional<Timestamp> find_nearest_checkpoint(Timestamp) const;
    std::optional<Timestamp> get_latest_checkpoint() const;
    std::optional<Timestamp> get_earliest_checkpoint() const;

    size_t get_checkpoint_count() const;

    void clear_all_checkpoint();

    bool has_checkpoint(Timestamp) const;
};