#pragma once

#include <vector>
#include <memory>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"

namespace mmre {
namespace query {

/**
 * @brief Interface for cursor-based streaming of large query result sets.
 * Prevents memory explosion (OOM) by allowing chunked/paginated retrieval
 * rather than loading an entire timeline range into memory simultaneously.
 */
class IQueryCursor {
public:
    virtual ~IQueryCursor() = default;

    /**
     * @brief Checks if more data is available in the result set.
     */
    virtual bool HasNext() const = 0;

    /**
     * @brief Fetches the next batch of results up to batchSize.
     * @param batchSize The maximum number of snapshots to retrieve in this call.
     * @return A vector of snapshots up to batchSize length.
     */
    virtual std::vector<engine_core::DataSnapshot> FetchNextBatch(size_t batchSize = 100) = 0;
};

} // namespace query
} // namespace mmre