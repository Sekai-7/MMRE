#pragma once

#include <string>
#include <memory>
#include <vector>
#include "mmre/common/Types.hpp"
#include "mmre/common/SharedMemoryHandle.hpp"

namespace mmre {
namespace query {

struct QueryRequest {
    std::string resourceId;
    std::string subResourceId;
    common::TimestampNs startTime;
    common::TimestampNs endTime;
    bool isSnapshot{false};
    bool requestExplainable{false}; ///< True if the Agent wants semantic text instead of raw data
};

// Forward declaration
namespace mmre { namespace engine_core { class TimelineCache; } }

/**
 * @brief The Query Execution Engine.
 * 
 * // [架构优化说明]
 * // 将底层的 `TimelineCache` 查找逻辑与上层的“格式化模板（如 JSON）”解耦。
 * // 专门提供两条通路：快路径（直接返回 Zero-Copy 句柄）与慢路径（走大模型所需的语义转译 JSON）。
 */
class QueryEngine {
public:
    explicit QueryEngine(std::shared_ptr<engine_core::TimelineCache> cache);

    /**
     * @brief Executes query and serializes the result into a formatted JSON string.
     * Suitable for Vehicle Signals and System Logs.
     */
    std::string ExecuteQueryAsJson(const QueryRequest& request);

    /**
     * @brief Fast-path query for large blobs. Returns just the shared memory handles.
     */
    std::vector<common::SharedMemoryHandle> ExecuteQueryForBlob(const QueryRequest& request);

private:
    std::shared_ptr<engine_core::TimelineCache> cache_;
};

} // namespace query
} // namespace mmre
