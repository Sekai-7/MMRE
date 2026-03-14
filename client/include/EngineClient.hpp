#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "Types.hpp"
#include "SharedMemoryPtr.hpp"

// 引入客户端前置解析器，实现胖客户端架构
namespace mmre { namespace query { class InterfaceParser; } }

namespace mmre {
namespace client {

/**
 * @brief 包含多模态数据的富查询结果 DTO。
 */
struct SqlQueryResult {
    std::string textualPayload;
    std::vector<memory::SharedMemoryPtr> blobData; // SDK 内部自动完成映射，直接提供可用指针
};

/**
 * @brief 触发器事件的多模态载荷 DTO。
 */
struct TriggerEvent {
    std::string eventMetadata;
    std::vector<memory::SharedMemoryPtr> shmData; // 零拷贝数据，例如触发时的 Camera 帧
};

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
     * @note SDK 内部在此阶段执行字符串到 Bytecode 的重度 CPU 编译工作，
     * 仅将编译后的 Bytecode 发送给 Server，彻底消除 IPC 总线与服务端的计算阻塞。
     */
    SqlQueryResult QuerySql(const std::string& sqlQuery);

    /**
     * @brief Fast-path for large Blob range query. Returns zero-copy handles.
     */
    std::vector<memory::SharedMemoryPtr> QueryBlobRange(const std::string& resourceId, 
                                                        common::TimestampNs start, 
                                                        common::TimestampNs end);

    /**
     * @brief Subscribes to an active trigger event (e.g. "Fatigue_Detected").
     */
    void SubscribeTrigger(const std::string& eventName, std::function<void(const TriggerEvent&)> callback);

    /**
     * @brief [架构补充] Inject a semantic checkpoint manually driven by higher-level Agent inference.
     */
    void InjectCheckpoint(const std::string& description);

private:
    std::string connectionUri_;
    // 客户端持有的查询编译器实例
    std::shared_ptr<mmre::query::InterfaceParser> sqlCompiler_;
};

} // namespace client
} // namespace mmre
