#pragma once

#include <atomic>
#include <cstdint>
#include "UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Internal wrapper for TimelineCache intrusive structures.
 * Decouples actual data from the storage container internals (SRP).
 */
struct alignas(64) CacheNode {
    UnifiedDataPacket data;
    std::atomic<bool> isPersisted{false};
    bool isCheckpoint{false};

    // Intrusive Pointers managed strictly by Cache logic
    CacheNode* prev{nullptr};
    CacheNode* next{nullptr};
    CacheNode* parent{nullptr};
    CacheNode* left{nullptr};
    CacheNode* right{nullptr};
    uint8_t color{0}; // 0: Red, 1: Black
};

} // namespace engine_core
} // namespace mmre