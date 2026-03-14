#pragma once

#include <cstdint>
#include <variant>
#include <atomic>
#include "Types.hpp"
#include "SharedMemoryPtr.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief O(1) Copyable data snapshot for fast-path reading.
 * Guarantees zero-copy for large blobs via SharedMemoryPtr ref-counting,
 * whilst keeping primitive signals extremely fast to transmit.
 */
struct DataSnapshot {
    common::TimestampNs timestamp;
    common::ResourceType type;
    std::variant<double, memory::SharedMemoryPtr, common::SmallString> payload;
};

/**
 * @brief Intrusive node representation of multi-modal data.
 * ABSOLUTELY NO dynamic allocation allowed inside this struct.
 */
struct alignas(64) UnifiedDataPacket {
    common::TimestampNs timestamp;
    common::ResourceType type;
    
    uint32_t resourceIdHash;    
    uint32_t subResourceIdHash; 

    // Payload polymorphism
    std::variant<double, memory::SharedMemoryPtr, common::SmallString> payload;

    std::atomic<bool> isPersisted{false};
    bool isCheckpoint{false};

    // Intrusive Doubly-Linked List (For O(1) tail appends and temporal locality)
    UnifiedDataPacket* prev{nullptr};
    UnifiedDataPacket* next{nullptr};
    
    // Intrusive Red-Black Tree Pointers (For O(logN) historical lookups)
    UnifiedDataPacket* parent{nullptr};
    UnifiedDataPacket* left{nullptr};
    UnifiedDataPacket* right{nullptr};
    uint8_t color{0}; // 0: Red, 1: Black
};

} // namespace engine_core
} // namespace mmre