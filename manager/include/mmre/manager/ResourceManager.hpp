#pragma once

#include <memory>
#include <string>
#include "mmre/common/Config.hpp"
#include "mmre/engine_core/TimelineCache.hpp"
#include "mmre/engine_core/PersistenceEngine.hpp"
#include "mmre/engine_core/TriggerManager.hpp"
#include "mmre/engine_core/CheckpointManager.hpp"
#include "mmre/ingestion/DataIngestionManager.hpp"
#include "mmre/query/QueryEngine.hpp"
#include "mmre/communication/IServer.hpp"

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

    void Initialize();
    void Run();

private:
    std::string HandleClientRequest(const std::string& payload);

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
