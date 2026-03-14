#pragma once

#include "mmre/communication/IServer.hpp"
#include <string>

namespace mmre {
namespace communication {

/**
 * @brief FDBus implementation for cross-domain RPC communication.
 * 
 * // [架构优化说明]
 * // 座舱架构通常划分为 IVI 域与 ADAS 域。通过引入 FDBus 支持跨节点透明访问。
 */
class FdbusRpcServer : public IServer {
public:
    explicit FdbusRpcServer(const std::string& serviceName);
    ~FdbusRpcServer() override;

    void Start() override;
    void Stop() override;
    void SetCallback(RequestCallback callback) override;

private:
    std::string serviceName_;
    RequestCallback callback_;
    // FDBus specific context
};

} // namespace communication
} // namespace mmre
