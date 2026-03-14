#pragma once

#include <memory>
#include <string>
#include "UnifiedDataPacket.hpp"
#include "Types.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Strategy pattern interface for intelligent data downsampling during persistence.
 * 
 * // [架构优化说明] 
 * // 针对 UFS 空间受限问题，不能全量转存�?
 * // 不同模态必须有专门的策略，例如：Camera 需要抽帧，Audio 需要特征提取或降采样�?
 */
class IDataStoreStrategy {
public:
    virtual ~IDataStoreStrategy() = default;
    
    /**
     * @brief Process packet before writing to disk (compress, sub-sample, or drop).
     * @return Processed packet, or nullptr if the packet should be dropped.
     */
    virtual std::shared_ptr<UnifiedDataPacket> ProcessForStorage(std::shared_ptr<UnifiedDataPacket> packet) = 0;
};

/**
 * @brief Factory/Manager for resolving persistence strategies based on Resource Type.
 */
class PersistencePolicyManager {
public:
    PersistencePolicyManager();
    std::shared_ptr<IDataStoreStrategy> GetStrategy(common::ResourceType type);
    
    // Registers a specific strategy (e.g. CameraFrameSubsamplingStrategy)
    void RegisterStrategy(common::ResourceType type, std::shared_ptr<IDataStoreStrategy> strategy);
};

} // namespace engine_core
} // namespace mmre
