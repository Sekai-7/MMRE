#pragma once

#include <cstdint>
#include <string_view>
#include <cstring>

namespace mmre {
namespace common {

/**
 * @brief Open-Closed Principle: ResourceType as uint32_t allows dynamic registration
 * of new modalities without changing core headers.
 */
using ResourceType = uint32_t;

namespace ResourceTypes {
    constexpr ResourceType VEHICLE_SIGNAL = 0;
    constexpr ResourceType CAMERA_FRAME = 1;
    constexpr ResourceType AUDIO_STREAM = 2;
    constexpr ResourceType SCREEN_CAPTURE = 3;
    constexpr ResourceType SYSTEM_LOG = 4;
    constexpr ResourceType UNKNOWN = 255;
}

/**
 * @brief Nanosecond-precision timestamp for unified time alignment.
 * 建议在硬件层面与 CLOCK_REALTIME 或系统全局 PTP 时钟同步。
 */
using TimestampNs = uint64_t;

/**
 * @brief Zero-allocation small string buffer for system logs / explainable text.
 * Fits well within standard cache lines.
 */
struct SmallString {
    static constexpr size_t MAX_LEN = 55;
    char data[MAX_LEN];
    uint8_t len{0};

    void assign(std::string_view sv) {
        len = static_cast<uint8_t>(sv.length() > MAX_LEN ? MAX_LEN : sv.length());
        std::memcpy(data, sv.data(), len); // Standard cross-platform implementation
    }
};

/**
 * @brief Generic buffer for inlining small payloads (e.g. primitives, strings) 
 * without heap allocation, avoiding rigid std::variant types.
 * Moved to common layer to decouple HAL ingestion from engine_core.
 */
struct PayloadBuffer {
    uint32_t typeTag{0}; // Runtime type identification (RTTI) tag to prevent unsafe casting
    uint32_t size{0};
    uint8_t data[56]{0}; // Kept to 56 bytes to maintain 64-byte total alignment
};

/**
 * @brief Granular error codes to prevent semantic loss across subsystem boundaries.
 */
enum class StatusCode : uint8_t {
    SUCCESS = 0,
    ERR_OOM_CACHE_POOL = 1,
    ERR_STALE_TIMESTAMP = 2,
    ERR_SHM_MAPPING_FAILED = 3,
    ERR_PROVIDER_INIT_FAILED = 4,
    ERR_INVALID_CONFIG = 5,
    ERR_UFS_IO_TIMEOUT = 6,
    ERR_PREDICATE_EVAL_FAILED = 7
};

/**
 * @brief Unified return status for operations to replace coarse-grained bool returns.
 * Fixed: Replaced dangling `const char*` pointer with inline buffer for complete RAII safety.
 */
struct SystemStatus {
    StatusCode code{StatusCode::SUCCESS};
    char message[64]{0};

    bool IsSuccess() const { return code == StatusCode::SUCCESS; }
    static SystemStatus Success() { return {StatusCode::SUCCESS, ""}; }
    static SystemStatus Error(StatusCode c, const char* msg) { 
        SystemStatus s;
        s.code = c;
        if (msg) {
            std::strncpy(s.message, msg, sizeof(s.message) - 1);
            s.message[sizeof(s.message) - 1] = '\0';
        }
        return s;
    }
};

} // namespace common
} // namespace mmre