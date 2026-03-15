#pragma once

#include <cstddef>
#include <cstdint>

// Forward declaration from common layer to avoid tight coupling.
namespace tl
{
    struct ResourceFrame;
}

// C-compatible API for agents.
#ifdef __cplusplus
extern "C" {
#endif

// Status codes for client operations.
typedef enum tl_status
{
    TL_STATUS_OK = 0,
    TL_STATUS_ERROR_GENERIC = 1,
    TL_STATUS_ERROR_INVALID_ARGUMENT = 2,
    TL_STATUS_ERROR_CONNECTION = 3,
    TL_STATUS_ERROR_NOT_INITIALIZED = 4
} tl_status_t;

// Opaque client context type.
typedef struct tl_client_context tl_client_context_t;

// Opaque resource handle used for shared memory or other external resources.
typedef std::uint64_t tl_handle_t;

// Callback used to receive query or subscription results.
typedef void (*tl_frame_callback_t)(const tl::ResourceFrame* frame, void* user_data);

// Initialize client library and create a client context.
// config_uri can be a path or logical URI describing how to reach the server.
tl_status_t tl_init(tl_client_context_t** out_ctx, const char* config_uri);

// Shut down client and release resources.
void tl_shutdown(tl_client_context_t* ctx);

// Query resources within [start_ts_ns, end_ts_ns] (inclusive) and invoke cb
// for each matching frame.
tl_status_t tl_query_range(
    tl_client_context_t* ctx,
    std::int64_t start_ts_ns,
    std::int64_t end_ts_ns,
    tl_frame_callback_t cb,
    void* user_data);

// Query latest state for a given filter expression (implementation-defined).
tl_status_t tl_query_latest(
    tl_client_context_t* ctx,
    const char* filter_expression,
    tl_frame_callback_t cb,
    void* user_data);

// Subscribe to trigger events based on a trigger expression.
tl_status_t tl_subscribe(
    tl_client_context_t* ctx,
    const char* trigger_expression,
    tl_frame_callback_t cb,
    void* user_data);

// Create a checkpoint with a logical name and optional timestamp / metadata.
tl_status_t tl_create_checkpoint(
    tl_client_context_t* ctx,
    const char* name,
    std::int64_t timestamp_ns,
    const char* metadata_json);

// Map a resource handle into the process address space. The underlying
// mechanism is platform-independent from the client's perspective.
tl_status_t tl_map_handle(
    tl_client_context_t* ctx,
    tl_handle_t handle,
    void** mapped_addr,
    std::size_t* mapped_size);

// Unmap a previously mapped handle.
tl_status_t tl_unmap_handle(
    tl_client_context_t* ctx,
    void* mapped_addr,
    std::size_t mapped_size);

#ifdef __cplusplus
} // extern "C"
#endif

