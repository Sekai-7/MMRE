#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include "Types.hpp"
#include "SharedMemoryHandle.hpp"

namespace mmre {
namespace communication {

/**
 * @brief Structured response allowing zero-copy transmission of SHM handles
 * alongside formatting metadata and textual payloads (DTO Pattern).
 */
struct IpcResponse {
    common::StatusCode status;
    std::string textualPayload; // Could be JSON metadata, plugin-formatted string, or raw error string
    std::vector<memory::SharedMemoryHandle> shmHandles; // Zero-copy data handles
};

/**
 * @brief Abstract base class for communication servers.
 * 
 * // [架构优化说明]
 * // 将控制面与数据面完全分离。无论底层是 Unix Domain Socket 还是 FDBus RPC，
 * // 均通过此抽象接口注入到引擎层。实现协议透明与解耦。
 */
class IServer {
public:
    // Fixed: Strongly typed binary payload for IPC instead of std::string to avoid 
    // serialization/deserialization overhead and preserve type safety.
    using RequestCallback = std::function<IpcResponse(const std::vector<uint8_t>& requestPayload)>;

    virtual ~IServer() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetCallback(RequestCallback callback) = 0;

    /**
     * @brief Pushes an asynchronous event directly to connected Agents.
     * Fixed: Added 'topic' for pub/sub routing to prevent IPC broadcast storms.
     * Fixed: Added DTO separation allowing shared memory handles to be passed 
     * out of process without leaking local DataSnapshot/SharedMemoryPtr memory contexts.
     * 
     * @param topic Routing topic (e.g. Agent ID, Resource Type, or Event Name)
     * @param eventPayload Binary metadata for the event
     * @param shmHandles Zero-copy data handles for the event (if any)
     */
    virtual void PushEvent(const std::string& topic, 
                           const std::vector<uint8_t>& eventPayload,
                           const std::vector<memory::SharedMemoryHandle>& shmHandles) = 0;
};

} // namespace communication
} // namespace mmre
