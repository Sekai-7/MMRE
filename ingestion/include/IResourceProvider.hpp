#pragma once

#include <string>
#include <functional>
#include <variant>
#include "Types.hpp"
#include "SharedMemoryPtr.hpp"

namespace mmre {
namespace ingestion {

/**
 * @brief Hardware-agnostic representation of ingested data,
 * preventing dependency inversion from core engine types.
 */
struct RawProviderData {
    common::TimestampNs timestamp;
    common::ResourceType type;
    uint32_t resourceIdHash;
    uint32_t subResourceIdHash;
    uint32_t pluginSchemaId{0};
    
    // Aligns with OCP extensibility buffer, fully decoupled from engine_core
    std::variant<common::PayloadBuffer, memory::SharedMemoryPtr> payload;
};

/**
 * @brief Callback invoked by a Resource Provider when new hardware data is generated.
 * Passes RawProviderData by rvalue reference to enforce zero-copy from the very edge.
 */
using OnDataPushedCallback = std::function<void(RawProviderData&&)>;

/**
 * @brief Hardware Abstraction Layer for any multi-modal input source.
 */
class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;

    /**
     * @brief Initialize provider with specific configuration.
     * Returns granular SystemStatus instead of coarse boolean.
     */
    virtual common::SystemStatus Initialize(const std::string& configJson) = 0;

    /**
     * @brief Register the callback to push data into the Engine's channels.
     */
    virtual void SetPushCallback(OnDataPushedCallback callback) = 0;

    /**
     * @brief Start capturing hardware data on a specific fine-grained channel.
     * Fixed: Added channelConfigJson to allow Agent to control dynamic hardware parameters
     * (e.g. resolution, FPS, sampling rate) avoiding a completely blind channel start.
     */
    virtual void StartChannel(uint32_t subResourceIdHash, const std::string& channelConfigJson) = 0;

    /**
     * @brief Stop capturing on a specific fine-grained channel.
     */
    virtual void StopChannel(uint32_t subResourceIdHash) = 0;
};

} // namespace ingestion
} // namespace mmre
