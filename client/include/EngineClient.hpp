#pragma once

#include <string>
#include <functional>
#include "Types.hpp"
#include "SharedMemoryHandle.hpp"

namespace mmre {
namespace client {

/**
 * @brief Client SDK (.so/.dll) for System Agents / OEM Agents to interact with the Engine.
 */
class EngineClient {
public:
    explicit EngineClient(const std::string& connectionUri);
    ~EngineClient();

    bool Connect();

    /**
     * @brief Executes a formatted SQL-like query. (e.g. "SELECT * FROM Camera WHERE time=NOW")
     */
    std::string QuerySql(const std::string& sqlQuery);

    /**
     * @brief Fast-path for large Blob range query. Returns zero-copy handles.
     */
    std::vector<common::SharedMemoryHandle> QueryBlobRange(const std::string& resourceId, 
                                                           common::TimestampNs start, 
                                                           common::TimestampNs end);

    /**
     * @brief Subscribes to an active trigger event (e.g. "Fatigue_Detected").
     */
    void SubscribeTrigger(const std::string& eventName, std::function<void(const std::string&)> callback);

    /**
     * @brief [架构补充] Inject a semantic checkpoint manually driven by higher-level Agent inference.
     */
    void InjectCheckpoint(const std::string& description);

private:
    std::string connectionUri_;
};

} // namespace client
} // namespace mmre
