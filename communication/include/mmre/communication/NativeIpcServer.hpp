#pragma once

#include "mmre/communication/IServer.hpp"
#include <string>

namespace mmre {
namespace communication {

/**
 * @brief Unix Domain Socket implementation of the server for low-latency same-node IPC.
 */
class NativeIpcServer : public IServer {
public:
    explicit NativeIpcServer(const std::string& socketPath);
    ~NativeIpcServer() override;

    void Start() override;
    void Stop() override;
    void SetCallback(RequestCallback callback) override;

private:
    std::string socketPath_;
    RequestCallback callback_;
    // TODO: UDS file descriptor and connection handling
};

} // namespace communication
} // namespace mmre
