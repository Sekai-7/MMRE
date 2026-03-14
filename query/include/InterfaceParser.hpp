#pragma once

#include <string>
#include "QueryEngine.hpp"

namespace mmre {
namespace query {

/**
 * @brief Adapter for translating external protocols into internal Standard Query Requests.
 */
class InterfaceParser {
public:
    /**
     * @brief Parses an SQL-like query string.
     */
    static QueryRequest ParseSql(const std::string& sqlQuery);

    /**
     * @brief Parses FDBus or native IPC RPC payloads.
     */
    static QueryRequest ParseRpcPayload(const std::string& rpcPayload);
};

} // namespace query
} // namespace mmre
