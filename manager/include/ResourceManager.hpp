#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "Config.hpp"
#include "Types.hpp"
#include "TimelineCache.hpp"
#include "PersistenceEngine.hpp"
#include "TriggerManager.hpp"
#include "CheckpointManager.hpp"
#include "DataIngestionManager.hpp"
#include "QueryEngine.hpp"
#include "IServer.hpp"

namespace mmre {
namespace manager {

/**
 * @brief Central Controller for the Multi-Modal Resource Engine.
 * 
 * // [架构优化说明]
 * // Facade 模式整合所有核心组件。统一管理配置、通信服务器绑定及子模块的生命周期启停。
 */
class ResourceManager {
public:
    explicit ResourceManager(const common::EngineConfig& config);
    ~ResourceManager();

    /**
     * @brief Initializes the entire system, returning a specific status code on failure.
     */
    common::SystemStatus Initialize();

    /**
     * @brief Starts all internal reactors and event loops.
     */
    common::SystemStatus Run();

private:
    communication::IpcResponse HandleClientRequest(const std::vector<uint8_t>& payload);

    common::EngineConfig config_;

    std::shared_ptr<engine_core::TimelineCache> cache_;
    std::shared_ptr<engine_core::PersistenceEngine> persistence_;
    std::shared_ptr<engine_core::TriggerManager> triggers_;
    std::shared_ptr<engine_core::CheckpointManager> checkpointMgr_;

    std::shared_ptr<query::QueryEngine> queryEngine_;
    std::shared_ptr<ingestion::DataIngestionManager> ingestion_;

    std::vector<std::unique_ptr<communication::IServer>> servers_;
};

} // namespace manager
} // namespace mmre
