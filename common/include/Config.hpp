#pragma once

#include <cstdint>
#include <string>

namespace mmre {
namespace common {

/**
 * @brief Global configuration for the Multi-Modal Resource Engine.
 */
struct EngineConfig {
    uint64_t maxCacheSizeMb{1024};             ///< Soft memory limit before async eviction
    uint64_t persistenceIntervalMs{5000};      ///< Scheduled sync interval
    uint64_t fallbackWindowNs{1000000000};     ///< Default 1s look-back for snapshot queries
    std::string defaultStoragePath{"/data/ufs/mmre"}; ///< UFS storage mount point
    bool enableCheckpoints{true};              ///< Whether checkpoint sync is enabled
};

} // namespace common
} // namespace mmre
