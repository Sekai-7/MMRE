#pragma once

#include <future>
#include <optional>
#include <vector>
#include <memory>
#include <functional>
#include "Types.hpp"
#include "TimelineCache.hpp"

namespace mmre {
namespace engine_core {
    class PersistenceEngine; // Forward declaration
}

namespace query {

struct QueryRequest {
    uint32_t resourceIdHash;
    uint32_t subResourceIdHash;
    common::TimestampNs startTime;
    common::TimestampNs endTime; // If equal to startTime, it acts as a point query
    bool isLatest{false};        // If true, bypasses time and fetches current state

    // Predicate pushdown: execute complex filtering (e.g. text search, value thresholds) 
    // at the storage/cache level before returning data over IPC.
    std::function<bool(const engine_core::DataSnapshot&)> filterPredicate{nullptr};
};

/**
 * @brief Facade mapping requests to Fast Path (RAM) or Slow Path (UFS).
 */
class QueryEngine {
public:
    QueryEngine(std::shared_ptr<engine_core::TimelineCache> cache, 
                std::shared_ptr<engine_core::PersistenceEngine> persistence);

    /**
     * @brief Orchestrates execution based on data locality.
     * If data is hot, resolves future immediately.
     * If data is cold, dispatches to UFS worker pool to prevent blocking Agent RPC.
     * Applies filterPredicate close to the data source to minimize memory bandwidth usage.
     */
    std::future<std::vector<engine_core::DataSnapshot>> ExecuteQuery(const QueryRequest& req);

private:
    std::shared_ptr<engine_core::TimelineCache> cache_;
    std::shared_ptr<engine_core::PersistenceEngine> persistence_;

    bool IsDataInCache(uint32_t resHash, uint32_t subHash, common::TimestampNs timeTarget) const;
};

} // namespace query
} // namespace mmre