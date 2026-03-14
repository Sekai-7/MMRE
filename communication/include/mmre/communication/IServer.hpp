#pragma once

#include <functional>
#include <string>

namespace mmre {
namespace communication {

/**
 * @brief Abstract base class for communication servers.
 * 
 * // [架构优化说明]
 * // 将控制面与数据面完全分离。无论底层是 Unix Domain Socket 还是 FDBus RPC，
 * // 均通过此抽象接口注入到引擎层。实现协议透明与解耦。
 */
class IServer {
public:
    using RequestCallback = std::function<std::string(const std::string& requestPayload)>;

    virtual ~IServer() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetCallback(RequestCallback callback) = 0;
};

} // namespace communication
} // namespace mmre
