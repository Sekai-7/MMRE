#pragma once

#include <cstdint>
#include <variant>
#include "Types.hpp"
#include "SharedMemoryPtr.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Generic buffer for inlining small payloads (e.g. primitives, strings) 
 * without heap allocation, avoiding rigid std::variant types.
 */
struct PayloadBuffer {
    uint8_t data[64]{0};
    uint32_t size{0};
};

/**
 * @brief O(1) Copyable data snapshot for fast-path reading.
 * Guarantees zero-copy for large blobs via SharedMemoryPtr ref-counting.
 */
struct DataSnapshot {
    common::TimestampNs timestamp;
    common::ResourceType type;
    uint32_t pluginSchemaId{0}; // Identifies the plugin to explain/format this data
    
    // Extensible Open-Closed polymorphic payload
    std::variant<PayloadBuffer, memory::SharedMemoryPtr> payload;
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
    uint32_t pluginSchemaId{0}; 

    std::variant<PayloadBuffer, memory::SharedMemoryPtr> payload;
};

} // namespace engine_core
} // namespace mmre