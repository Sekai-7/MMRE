#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include "Types.hpp"
#include "SharedMemoryHandle.hpp"
#include "SharedMemoryPtr.hpp" // 【重构引入】必须感知生命周期

namespace mmre {
namespace communication {

/**
 * @brief Structured response strictly enforcing cross-process RAII.
 */
struct IpcResponse {
    common::StatusCode status;
    std::string textualPayload; // Could be JSON metadata, plugin-formatted string, or raw error string
    // 【重构：生命周期拦截】必须持有 SharedMemoryPtr，而不是裸句柄 (Handle)。
    // Server 底层(如 UDS/FDBus 协议栈)将持有此对象，直到收到对端 ACK 才允许析构。
    std::vector<memory::SharedMemoryPtr> retainedMemory; 
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
    using ReplyCallback = std::function<void(const IpcResponse& response)>;

    // Fixed: Transformed from synchronous return to asynchronous continuation (ReplyCallback).
    // This strictly prevents long-tail operations (like UFS disk seeks) from blocking 
    // the networking IO Reactor threads.
    using RequestCallback = std::function<void(const std::vector<uint8_t>& requestPayload, ReplyCallback reply)>;

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
     * @param retainedMemory 生命期强引用的智能指针集合，确保对方读取前不被 UFS 回收
     */
    virtual void PushEvent(const std::string& topic, 
                           const std::vector<uint8_t>& eventPayload,
                           const std::vector<memory::SharedMemoryPtr>& retainedMemory) = 0;
};

} // namespace communication
} // namespace mmre
