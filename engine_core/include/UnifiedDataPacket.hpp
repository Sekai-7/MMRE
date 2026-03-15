#pragma once

#include <cstdint>
#include "Types.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief O(1) Copyable data snapshot for fast-path reading.
 * Guarantees zero-copy for large blobs via SharedMemoryPtr ref-counting.
 */
struct DataSnapshot {
    common::TimestampNs timestamp;
    common::ResourceType type;
    
    // 【重构】使用统一的类型别名，彻底剥离表现层元数据
    common::PayloadVariant payload;
};

/**
 * @brief Pure data representation of multi-modal data.
 * Adheres to SRP by strictly isolating data structure from caching algorithms.
 */
struct UnifiedDataPacket {
    common::TimestampNs timestamp;
    common::ResourceType type;
    
    uint32_t resourceIdHash;    
    uint32_t subResourceIdHash; 

    // 【重构】纯粹的数据载荷变体
    common::PayloadVariant payload;
};

} // namespace engine_core
} // namespace mmre