#pragma once

#include <cstdint>

namespace tl
{
    // Simple binary protocol shared between client and server for UDS.

    constexpr std::uint32_t kIpcMagic   = 0x544C5245; // 'TLRE'
    constexpr std::uint16_t kIpcVersion = 1;

    enum class IpcMessageType : std::uint16_t
    {
        QueryLatestRequest  = 1,
        QueryLatestResponse = 2,
        ErrorResponse       = 3
    };

    struct IpcHeader
    {
        std::uint32_t magic;
        std::uint16_t version;
        std::uint16_t type;
        std::uint32_t payload_size;
    };

    // Wire representation of ResourceFrame for IPC. Metadata is omitted
    // in this minimal prototype and may be added later if needed.
    struct IpcResourceFrame
    {
        std::int64_t  timestamp_ns;
        std::uint8_t  type;
        std::uint8_t  reserved[7]{};
        std::uint64_t handle_id;
        std::uint32_t source_size;
        // Followed by `source_size` bytes of UTF-8 source_id.
    };
} // namespace tl

