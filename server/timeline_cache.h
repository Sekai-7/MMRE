#pragma once

#include <cstdint>
#include <algorithm>
#include <mutex>
#include <optional>
#include <vector>

#include "resource_frame.h"

namespace tl
{
    // Very simple in-memory timeline cache that stores ResourceFrame
    // instances ordered by timestamp and allows point-in-time lookup.
    class TimelineCache
    {
    public:
        TimelineCache() = default;

        // Insert a frame into the cache. Frames are stored ordered by
        // timestamp_ns in ascending order.
        void insert(ResourceFrame frame)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = lower_bound_position(frame.timestamp_ns);
            frames_.insert(it, std::move(frame));
        }

        // Point-in-time lookup: find the latest frame with timestamp
        // <= query_ts_ns. Returns std::nullopt if none exists.
        std::optional<ResourceFrame> query_point(std::int64_t query_ts_ns) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (frames_.empty())
            {
                return std::nullopt;
            }

            std::size_t left = 0;
            std::size_t right = frames_.size();

            // Binary search for the first element with timestamp > query_ts_ns.
            while (left < right)
            {
                const std::size_t mid = left + (right - left) / 2;
                if (frames_[mid].timestamp_ns <= query_ts_ns)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }

            if (left == 0)
            {
                return std::nullopt;
            }

            return frames_[left - 1];
        }

        // Latest frame in the cache (highest timestamp), if any.
        std::optional<ResourceFrame> query_latest() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (frames_.empty())
            {
                return std::nullopt;
            }
            return frames_.back();
        }

        // Number of frames currently stored.
        std::size_t size() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return frames_.size();
        }

    private:
        using FrameContainer = std::vector<ResourceFrame>;

        FrameContainer::iterator lower_bound_position(std::int64_t ts)
        {
            return std::lower_bound(
                frames_.begin(),
                frames_.end(),
                ts,
                [](const ResourceFrame& f, std::int64_t value) {
                    return f.timestamp_ns < value;
                });
        }

        mutable std::mutex mutex_;
        FrameContainer      frames_;
    };
} // namespace tl

