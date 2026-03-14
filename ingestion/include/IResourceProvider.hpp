#pragma once

#include <string>
#include <functional>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"

namespace mmre {
namespace ingestion {

/**
 * @brief Callback invoked by a Resource Provider when new hardware data is generated.
 * Passes DataSnapshot by value/rvalue reference to enforce zero-copy from the very edge
 * without doing dynamic memory allocations for cache nodes here.
 */
using OnDataPushedCallback = std::function<void(engine_core::DataSnapshot&&)>;

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
     * @brief Start capturing hardware data.
     */
    virtual void Start() = 0;

    /**
     * @brief Stop capturing and release hardware resources.
     */
    virtual void Stop() = 0;
};

} // namespace ingestion
} // namespace mmre
