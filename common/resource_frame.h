#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace tl
{
    // High-level resource categories supported by the engine.
    enum class ResourceType : std::uint8_t
    {
        Signal,
        Camera,
        Audio,
        Screen,
        Log,
        Custom
    };

    // Opaque handle referring to externally managed binary data.
    struct ResourceHandle
    {
        std::uint64_t id{0};
    };

    using ResourceMetadata = std::unordered_map<std::string, std::string>;

    // Core timeline resource unit.
    struct ResourceFrame
    {
        // Monotonic timestamp in nanoseconds since epoch or system-defined origin.
        std::int64_t      timestamp_ns{0};
        ResourceType      type{ResourceType::Custom};
        std::string       source_id;
        ResourceMetadata  metadata;
        ResourceHandle    handle;
    };
} // namespace tl

