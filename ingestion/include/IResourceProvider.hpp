
#pragma once

#include <string>
#include <functional>
#include <variant>
#include <memory>
#include "Types.hpp"

namespace mmre {
namespace ingestion {

/**
 * @brief 硬件无关的原生内存块抽象。
 * 允许 Provider 直接操作内存，但对底层实现（堆内存或共享内存）完全无感。
 */
struct ProviderBuffer {
    void* data{nullptr};
    size_t capacity{0};
    size_t filledSize{0}; // Provider 写入数据后更新此字段
};

/**
 * @brief 注入给 Provider 的分配器接口，由 Engine 实现并传入。
 * 实现控制反转 (IoC)，完美解耦 HAL 与 内存池。
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;
    virtual ProviderBuffer Allocate(size_t sizeHint) = 0;
};

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
    
    // 解耦完成：使用 ProviderBuffer 替代对核心 SharedMemoryPtr 的依赖
    std::variant<common::PayloadBuffer, ProviderBuffer> payload; 
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
     * 增加 allocator 参数，向硬件通道注入内存分配能力。
     */
    virtual void StartChannel(uint32_t subResourceIdHash, const std::string& channelConfigJson, 
                              std::shared_ptr<IAllocator> allocator) = 0;

    /**
     * @brief Stop capturing on a specific fine-grained channel.
     */
    virtual void StopChannel(uint32_t subResourceIdHash) = 0;
};

} // namespace ingestion
} // namespace mmre