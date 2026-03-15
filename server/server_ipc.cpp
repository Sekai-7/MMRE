#ifdef __linux__

#include "server_ipc.h"

#include <cerrno>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace tl
{
    namespace
    {
        bool read_exact(int fd, void* buf, std::size_t len)
        {
            auto* ptr = static_cast<std::uint8_t*>(buf);
            std::size_t total = 0;
            while (total < len)
            {
                const ssize_t n = ::read(fd, ptr + total, len - total);
                if (n <= 0)
                {
                    return false;
                }
                total += static_cast<std::size_t>(n);
            }
            return true;
        }

        bool write_exact(int fd, const void* buf, std::size_t len)
        {
            const auto* ptr = static_cast<const std::uint8_t*>(buf);
            std::size_t total = 0;
            while (total < len)
            {
                const ssize_t n = ::write(fd, ptr + total, len - total);
                if (n <= 0)
                {
                    return false;
                }
                total += static_cast<std::size_t>(n);
            }
            return true;
        }
    } // namespace

    ServerIpc::ServerIpc(TimelineCache& cache)
        : cache_(cache)
    {
    }

    ServerIpc::~ServerIpc()
    {
        stop();
    }

    bool ServerIpc::start(const std::string& socket_path)
    {
        if (running_.load())
        {
            return true;
        }

        socket_path_ = socket_path;

        listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (listen_fd_ < 0)
        {
            std::cerr << "Failed to create UDS socket: " << std::strerror(errno) << '\n';
            return false;
        }

        ::unlink(socket_path_.c_str());

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path_.c_str());

        if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            std::cerr << "Failed to bind UDS socket: " << std::strerror(errno) << '\n';
            ::close(listen_fd_);
            listen_fd_ = -1;
            return false;
        }

        if (::listen(listen_fd_, 4) < 0)
        {
            std::cerr << "Failed to listen on UDS socket: " << std::strerror(errno) << '\n';
            ::close(listen_fd_);
            listen_fd_ = -1;
            return false;
        }

        running_.store(true);
        thread_ = std::thread(&ServerIpc::run_loop, this);
        return true;
    }

    void ServerIpc::stop()
    {
        if (!running_.exchange(false))
        {
            return;
        }

        if (listen_fd_ >= 0)
        {
            ::close(listen_fd_);
            listen_fd_ = -1;
        }

        if (!socket_path_.empty())
        {
            ::unlink(socket_path_.c_str());
        }

        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    void ServerIpc::run_loop()
    {
        while (running_.load())
        {
            int client_fd = ::accept(listen_fd_, nullptr, nullptr);
            if (client_fd < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                if (!running_.load())
                {
                    break;
                }
                std::cerr << "Accept failed: " << std::strerror(errno) << '\n';
                continue;
            }

            handle_client(client_fd);
            ::close(client_fd);
        }
    }

    void ServerIpc::handle_client(int client_fd)
    {
        IpcHeader header{};
        if (!read_exact(client_fd, &header, sizeof(header)))
        {
            return;
        }

        if (header.magic != kIpcMagic || header.version != kIpcVersion)
        {
            send_error(client_fd, "Invalid IPC header");
            return;
        }

        if (static_cast<IpcMessageType>(header.type) == IpcMessageType::QueryLatestRequest)
        {
            // For now the payload (filter_expression) is ignored.
            if (header.payload_size > 0)
            {
                std::vector<std::uint8_t> sink(header.payload_size);
                if (!read_exact(client_fd, sink.data(), sink.size()))
                {
                    return;
                }
            }
            handle_query_latest(client_fd);
        }
        else
        {
            // Unknown message type.
            if (header.payload_size > 0)
            {
                std::vector<std::uint8_t> sink(header.payload_size);
                read_exact(client_fd, sink.data(), sink.size());
            }
            send_error(client_fd, "Unsupported message type");
        }
    }

    void ServerIpc::handle_query_latest(int client_fd)
    {
        const auto latest = cache_.query_latest();
        if (!latest)
        {
            send_error(client_fd, "No data in cache");
            return;
        }

        const std::string& source = latest->source_id;

        IpcHeader header{};
        header.magic        = kIpcMagic;
        header.version      = kIpcVersion;
        header.type         = static_cast<std::uint16_t>(IpcMessageType::QueryLatestResponse);
        header.payload_size = static_cast<std::uint32_t>(sizeof(IpcResourceFrame) + source.size());

        IpcResourceFrame frame{};
        frame.timestamp_ns = latest->timestamp_ns;
        frame.type         = static_cast<std::uint8_t>(latest->type);
        frame.handle_id    = latest->handle.id;
        frame.source_size  = static_cast<std::uint32_t>(source.size());

        if (!write_exact(client_fd, &header, sizeof(header)))
        {
            return;
        }
        if (!write_exact(client_fd, &frame, sizeof(frame)))
        {
            return;
        }
        if (!source.empty())
        {
            write_exact(client_fd, source.data(), source.size());
        }
    }

    void ServerIpc::send_error(int client_fd, const std::string& message)
    {
        IpcHeader header{};
        header.magic        = kIpcMagic;
        header.version      = kIpcVersion;
        header.type         = static_cast<std::uint16_t>(IpcMessageType::ErrorResponse);
        header.payload_size = static_cast<std::uint32_t>(message.size());

        if (!write_exact(client_fd, &header, sizeof(header)))
        {
            return;
        }
        if (!message.empty())
        {
            write_exact(client_fd, message.data(), message.size());
        }
    }
} // namespace tl

#endif // __linux__

