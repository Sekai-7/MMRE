#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace mmre {
namespace common {

/**
 * @brief Enum defining all supported multi-modal resource types.
 */
enum class ResourceType : uint8_t {
    VEHICLE_SIGNAL = 0,
    CAMERA_FRAME = 1,
    AUDIO_STREAM = 2,
    SCREEN_CAPTURE = 3,
    SYSTEM_LOG = 4,
    UNKNOWN = 255
};

/**
 * @brief Timestamp type alias (nanoseconds since epoch).
 * 统一使用纳秒级时间戳，满足高精度同步对齐需求。
 */
using TimestampNs = uint64_t;

/**
 * @brief Metadata map for Key-Value and Groupable features of Vehicle Signals.
 */
using MetadataMap = std::unordered_map<std::string, std::string>;

} // namespace common
} // namespace mmre
