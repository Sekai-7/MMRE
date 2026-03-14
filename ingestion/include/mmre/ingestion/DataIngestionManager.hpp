#pragma once

#include <memory>
#include "mmre/ingestion/LockFreeRingBuffer.hpp"
#include "mmre/engine_core/UnifiedDataPacket.hpp"

namespace mmre {
namespace ingestion {

/**
 * @brief Front-door for all incoming multi-modal data (Reactor Pattern).
 */
class DataIngestionManager {
public:
    DataIngestionManager();
    ~DataIngestionManager();

    /**
     * @brief Start the Reactor IO loop and worker threads.
     */
    void Start();

    /**
     * @brief Invoked by the I/O multiplexer (e.g. epoll) when raw data arrives.
     */
    void OnDataReceived(const uint8_t* rawData, size_t size);

private:
    LockFreeRingBuffer<std::shared_ptr<engine_core::UnifiedDataPacket>, 4096> ingestionQueue_;
    // TODO: Thread pool and epoll context definitions
};

} // namespace ingestion
} // namespace mmre
