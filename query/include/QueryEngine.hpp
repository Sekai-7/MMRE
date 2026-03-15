#pragma once

#include <future>
#include <optional>
#include <vector>
#include <memory>
#include <cstdint>
#include "QueryTypes.hpp"
#include "TimelineCache.hpp"

namespace mmre {
namespace engine_core {
    class PersistenceEngine; // Forward declaration
}

namespace query {

/**
 * @brief Facade mapping requests to Fast Path (RAM) or Slow Path (UFS).
 */
class QueryEngine {
public:
    QueryEngine(std::shared_ptr<engine_core::TimelineCache> cache, 
                std::shared_ptr<engine_core::PersistenceEngine> persistence);

    /**
     * @brief Orchestrates execution based on pre-compiled requests.
     */
    std::future<common::CursorResponse> ExecuteQuery(const common::QueryRequest& req);

    /**
     * @brief Fetches a batch of data tied to a specific session cursor across the IPC boundary.
     * @param cursorId The unique session identifier returned by ExecuteQuery.
     * @param batchSize The maximum number of elements to fetch.
     * @return A vector of snapshots. The Agent must convert these to DTOs in the communication layer.
     */
    std::vector<engine_core::DataSnapshot> FetchCursorBatch(uint64_t cursorId, size_t batchSize = 100);

    /**
     * @brief Closes the cursor and frees server-side resources.
     */
    void CloseCursor(uint64_t cursorId);

    /**
     * @brief 游标心跳保活。Agent 在长查询过程中调用以刷新 TTL，防止超时。
     */
    void KeepAliveCursor(uint64_t cursorId);

    /**
     * @brief 扫描并清理超时（TTL 过期）的游标。
     * 可由内部定时器或内存压力监测器异步触发，彻底杜绝崩溃导致的内存泄漏。
     */
    void EvictStaleCursors();

private:
    std::shared_ptr<engine_core::TimelineCache> cache_;
    std::shared_ptr<engine_core::PersistenceEngine> persistence_;

    bool IsDataInCache(uint32_t resHash, uint32_t subHash, common::TimestampNs timeTarget) const;
};

} // namespace query
} // namespace mmre