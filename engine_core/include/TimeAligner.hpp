#pragma once

#include <vector>
#include <memory>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Groups diverse sensor packets based on strictly bounded time-windows.
 * 
 * 解决 Camera (高延�? �?CAN (低延�? 到达时间错位的问题，确保落入 Cache 的数据具备严格物理时序�?
 */
class TimeAligner {
public:
    explicit TimeAligner(common::TimestampNs alignmentWindowNs = 50000000); // Default 50ms

    /**
     * @brief Inputs raw unordered packets.
     * @return Any packet groups that have successfully crossed the time-window barrier.
     */
    std::vector<std::shared_ptr<UnifiedDataPacket>> PushAndAlign(
        std::vector<std::unique_ptr<UnifiedDataPacket>> incomingPackets);

private:
    common::TimestampNs alignmentWindowNs_;
    common::TimestampNs currentHighWatermarkNs_{0};

    // Staging buffer holding packets until the time window matures
    std::vector<std::shared_ptr<UnifiedDataPacket>> stagingBuffer_;
};

} // namespace engine_core
} // namespace mmre
