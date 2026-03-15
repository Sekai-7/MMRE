#include "timeline_client.h"

#include <atomic>
#include <string>

#include "ipc_protocol.h"
#include "resource_frame.h"

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

// Internal definition of opaque client context.
struct tl_client_context
{
    std::string      config_uri;
    std::atomic<bool> initialized{false};

#ifdef __linux__
    int socket_fd{-1};
#endif
};

static bool is_valid_context(const tl_client_context* ctx)
{
    return ctx != nullptr && ctx->initialized.load();
}

tl_status_t tl_init(tl_client_context** out_ctx, const char* config_uri)
{
    if (out_ctx == nullptr)
    {
        return TL_STATUS_ERROR_INVALID_ARGUMENT;
    }

    auto* ctx = new (std::nothrow) tl_client_context{};
    if (!ctx)
    {
        return TL_STATUS_ERROR_GENERIC;
    }

    if (config_uri != nullptr)
    {
        ctx->config_uri = config_uri;
    }
    else
    {
        // Default UDS path used by the server.
        ctx->config_uri = "/tmp/timeline_resource_engine.sock";
    }

#ifdef __linux__
    ctx->socket_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (ctx->socket_fd < 0)
    {
        delete ctx;
        return TL_STATUS_ERROR_CONNECTION;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", ctx->config_uri.c_str());

    if (::connect(ctx->socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        ::close(ctx->socket_fd);
        ctx->socket_fd = -1;
        delete ctx;
        return TL_STATUS_ERROR_CONNECTION;
    }
#else
    // Non-Linux builds currently do not support UDS transport.
    delete ctx;
    return TL_STATUS_ERROR_CONNECTION;
#endif

    ctx->initialized.store(true);
    *out_ctx = ctx;
    return TL_STATUS_OK;
}

void tl_shutdown(tl_client_context* ctx)
{
    if (!ctx)
    {
        return;
    }

#ifdef __linux__
    if (ctx->socket_fd >= 0)
    {
        ::close(ctx->socket_fd);
        ctx->socket_fd = -1;
    }
#endif

    ctx->initialized.store(false);
    delete ctx;
}

tl_status_t tl_query_range(
    tl_client_context* ctx,
    std::int64_t /*start_ts_ns*/,
    std::int64_t /*end_ts_ns*/,
    tl_frame_callback_t /*cb*/,
    void* /*user_data*/)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Range query is not implemented in this prototype.
    return TL_STATUS_ERROR_GENERIC;
}

tl_status_t tl_query_latest(
    tl_client_context* ctx,
    const char* filter_expression,
    tl_frame_callback_t cb,
    void* user_data)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }
    if (!cb)
    {
        return TL_STATUS_ERROR_INVALID_ARGUMENT;
    }

#ifndef __linux__
    (void)filter_expression;
    (void)cb;
    (void)user_data;
    return TL_STATUS_ERROR_CONNECTION;
#else
    if (ctx->socket_fd < 0)
    {
        return TL_STATUS_ERROR_CONNECTION;
    }

    const std::string filter = filter_expression ? filter_expression : "";

    tl::IpcHeader header{};
    header.magic        = tl::kIpcMagic;
    header.version      = tl::kIpcVersion;
    header.type         = static_cast<std::uint16_t>(tl::IpcMessageType::QueryLatestRequest);
    header.payload_size = static_cast<std::uint32_t>(filter.size());

    auto write_exact = [](int fd, const void* buf, std::size_t len) -> bool {
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
    };

    if (!write_exact(ctx->socket_fd, &header, sizeof(header)))
    {
        return TL_STATUS_ERROR_CONNECTION;
    }
    if (!filter.empty())
    {
        if (!write_exact(ctx->socket_fd, filter.data(), filter.size()))
        {
            return TL_STATUS_ERROR_CONNECTION;
        }
    }

    auto read_exact = [](int fd, void* buf, std::size_t len) -> bool {
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
    };

    tl::IpcHeader resp_header{};
    if (!read_exact(ctx->socket_fd, &resp_header, sizeof(resp_header)))
    {
        return TL_STATUS_ERROR_CONNECTION;
    }
    if (resp_header.magic != tl::kIpcMagic || resp_header.version != tl::kIpcVersion)
    {
        return TL_STATUS_ERROR_GENERIC;
    }

    const auto msg_type = static_cast<tl::IpcMessageType>(resp_header.type);
    if (msg_type == tl::IpcMessageType::ErrorResponse)
    {
        if (resp_header.payload_size > 0)
        {
            std::string msg(resp_header.payload_size, '\0');
            if (!read_exact(ctx->socket_fd, msg.data(), msg.size()))
            {
                return TL_STATUS_ERROR_CONNECTION;
            }
            // For now, just print error to stderr.
            std::cerr << "Server error: " << msg << '\n';
        }
        return TL_STATUS_ERROR_GENERIC;
    }

    if (msg_type != tl::IpcMessageType::QueryLatestResponse)
    {
        // Drain payload if any, then report error.
        if (resp_header.payload_size > 0)
        {
            std::string sink(resp_header.payload_size, '\0');
            read_exact(ctx->socket_fd, sink.data(), sink.size());
        }
        return TL_STATUS_ERROR_GENERIC;
    }

    if (resp_header.payload_size < sizeof(tl::IpcResourceFrame))
    {
        return TL_STATUS_ERROR_GENERIC;
    }

    tl::IpcResourceFrame wire_frame{};
    if (!read_exact(ctx->socket_fd, &wire_frame, sizeof(wire_frame)))
    {
        return TL_STATUS_ERROR_CONNECTION;
    }

    const std::uint32_t remaining =
        resp_header.payload_size - static_cast<std::uint32_t>(sizeof(wire_frame));
    if (remaining != wire_frame.source_size)
    {
        // Consume remaining bytes and fail.
        if (remaining > 0)
        {
            std::string sink(remaining, '\0');
            read_exact(ctx->socket_fd, sink.data(), sink.size());
        }
        return TL_STATUS_ERROR_GENERIC;
    }

    std::string source_id;
    if (wire_frame.source_size > 0)
    {
        source_id.resize(wire_frame.source_size);
        if (!read_exact(ctx->socket_fd, source_id.data(), source_id.size()))
        {
            return TL_STATUS_ERROR_CONNECTION;
        }
    }

    tl::ResourceFrame frame;
    frame.timestamp_ns = wire_frame.timestamp_ns;
    frame.type         = static_cast<tl::ResourceType>(wire_frame.type);
    frame.source_id    = std::move(source_id);
    frame.handle.id    = wire_frame.handle_id;

    cb(&frame, user_data);
    return TL_STATUS_OK;
#endif
}

tl_status_t tl_subscribe(
    tl_client_context* ctx,
    const char* /*trigger_expression*/,
    tl_frame_callback_t /*cb*/,
    void* /*user_data*/)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Stub implementation: no-op subscription.
    return TL_STATUS_OK;
}

tl_status_t tl_create_checkpoint(
    tl_client_context* ctx,
    const char* /*name*/,
    std::int64_t /*timestamp_ns*/,
    const char* /*metadata_json*/)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Stub implementation: no-op checkpoint creation.
    return TL_STATUS_OK;
}

tl_status_t tl_map_handle(
    tl_client_context* ctx,
    tl_handle_t /*handle*/,
    void** mapped_addr,
    std::size_t* mapped_size)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }
    if (!mapped_addr || !mapped_size)
    {
        return TL_STATUS_ERROR_INVALID_ARGUMENT;
    }

    // Stub implementation: nothing mapped.
    *mapped_addr = nullptr;
    *mapped_size = 0U;
    return TL_STATUS_OK;
}

tl_status_t tl_unmap_handle(
    tl_client_context* ctx,
    void* /*mapped_addr*/,
    std::size_t /*mapped_size*/)
{
    if (!is_valid_context(ctx))
    {
        return TL_STATUS_ERROR_NOT_INITIALIZED;
    }

    // Stub implementation: nothing to unmap.
    return TL_STATUS_OK;
}

