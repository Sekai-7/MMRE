#pragma once

#include <string>

#include "resource_frame.h"

namespace tl
{
    // Abstract hardware-independent adapter interface consumed by the server.
    class ResourceAdapter
    {
    public:
        virtual ~ResourceAdapter() = default;

        // Identify the primary resource type produced by this adapter.
        virtual ResourceType resource_type() const noexcept = 0;

        // Logical identifier of the resource source (e.g., sensor name).
        virtual std::string source_id() const = 0;

        // Start producing resources. Returns false if startup fails.
        virtual bool start() = 0;

        // Stop producing resources and release internal resources.
        virtual void stop() noexcept = 0;

        // Poll for the next available frame. Returns true if a frame was
        // produced and stored in out_frame, false otherwise.
        //
        // Implementations must not perform hardware-specific logic outside
        // this abstraction boundary.
        virtual bool poll(ResourceFrame& out_frame) = 0;
    };
} // namespace tl

