#pragma once

#include <functional>
#include <vector>
#include <memory>
#include "Types.hpp"
#include "UnifiedDataPacket.hpp"

namespace mmre {
namespace engine_core {

using TriggerCallback = std::function<void(const DataSnapshot&)>;

/**
 * @brief Asynchronous Event-bus driven rule evaluator.
 * Decouples rule evaluation from the critical ingestion fast-path.
 */
class TriggerManager {
public:
    TriggerManager();
    ~TriggerManager();

    /**
     * @brief Registers a state machine transition rule from the Agent layer.
     */
    void RegisterRule(uint32_t resourceIdHash, std::function<bool(const DataSnapshot&)> predicate, TriggerCallback onFire);

    /**
     * @brief Non-blocking dispatch. Post events to a lock-free RingBuffer queue.
     * Consumed by an internal background Worker thread.
     */
    void DispatchEventAsync(const DataSnapshot& snapshot);

private:
    class TriggerWorker;
    std::unique_ptr<TriggerWorker> worker_; // Pimpl idiom to hide lock-free queue details
};

} // namespace engine_core
} // namespace mmre