#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "LockFreeRingBuffer.hpp"
#include "IResourceProvider.hpp"

namespace mmre {
namespace ingestion {

class DataIngestionManager {
public:
    DataIngestionManager();
    ~DataIngestionManager();

    /**
     * @brief Registers a hardware provider and creates a dedicated SPSC channel for it.
     */
    void RegisterProvider(uint32_t sourceId, std::unique_ptr<IResourceProvider> provider);

    void StartAll();
    void StopAll();

    /**
     * @brief Multi-channel polling method used by IO Reactor to drain all queues.
     * @return A batched vector of raw provider data.
     */
    std::vector<RawProviderData> PollChannels();

private:
    // 采用每数据源一队列 (Channel per source) 的隔离设计，绝对杜绝 SPSC 遭多线程破坏。
    // 使用 RawProviderData 传递以避免 Provider 侧对引擎核心数据结构的依赖。
    using ChannelQueue = LockFreeRingBuffer<RawProviderData, 4096>;
    
    std::unordered_map<uint32_t, std::unique_ptr<ChannelQueue>> channels_;
    std::unordered_map<uint32_t, std::unique_ptr<IResourceProvider>> providers_;
};

} // namespace ingestion
} // namespace mmre
