#pragma once

#include <string>
#include <functional>
#include <memory>
#include "mmre/engine_core/UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Manages rule-based event triggers to proactively wake up Agents.
 * 
 * // [架构优化说明]
 * // 将被动的“拉取（Pull）”变为主动的“推送（Push）”。
 * // Trigger 计算逻辑作为旁路（Bypass）任务挂载在工作线程末端，通过快速状态机匹配，
 * // 能够在微秒级生成 Checkpoint（检查点）或触发跨核事件，完全不干扰主数据流。
 */
class TriggerManager {
public:
    using TriggerCallback = std::function<void(const std::string& eventName, std::shared_ptr<UnifiedDataPacket>)>;

    TriggerManager() = default;
    ~TriggerManager() = default;

    /**
     * @brief Evaluates a newly ingested packet against registered rules.
     */
    void Evaluate(std::shared_ptr<UnifiedDataPacket> packet);

    /**
     * @brief Registers a callback for a specific semantic event (e.g., "Driver_Eyes_Closed").
     */
    void RegisterRule(const std::string& ruleDefinition, TriggerCallback callback);
};

} // namespace engine_core
} // namespace mmre
