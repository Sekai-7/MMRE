#pragma once

#include <string>
#include <memory>
#include "mmre/engine_core/UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Manages dynamic plugins to translate binary raw data into LLM-understandable text.
 * 
 * // [架构优化说明]
 * // 满足需求文档中反复提到的 "Explainable"。座舱大模型无法直接理解 CAN 原始 16 进制报文。
 * // 允许在运行时加载动态链接库 (.so) 插件，将状态翻译为语义描述，如 "Speed is 120km/h"。
 */
class PluginManager {
public:
    static PluginManager& GetInstance();

    /**
     * @brief Converts a given packet's payload into a semantic string if a plugin exists.
     */
    std::string Explain(std::shared_ptr<UnifiedDataPacket> packet);

    /**
     * @brief Dynamically load an explanation plugin.
     */
    bool LoadPlugin(const std::string& libraryPath, common::ResourceType type);

private:
    PluginManager() = default;
    ~PluginManager() = default;
    // Internal plugin registry mapping
};

} // namespace engine_core
} // namespace mmre
