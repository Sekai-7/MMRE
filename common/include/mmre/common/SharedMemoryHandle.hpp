#pragma once

#include <cstdint>
#include <string>

namespace mmre {
namespace common {

/**
 * @brief Handle for zero-copy shared memory access.
 * 
 * // [架构优化说明] 
 * // 采用进程间共享内存传递大载荷数据（如高清摄像头帧）。
 * // IPC 通信中仅传递此轻量级句柄（数十字节），彻底避免内核态与用户态的 Deep Copy，
 * // 满足单帧处理时间 <5ms 的极致性能要求。
 */
struct SharedMemoryHandle {
    uint32_t poolId;         ///< Identifier for the pre-allocated memory pool
    uint32_t blockId;        ///< Identifier for the specific block
    uint64_t offset;         ///< Offset within the shared memory region
    uint32_t size;           ///< Size of the actual data payload
    uint32_t referenceCount; ///< Cross-process reference count (managed in shared segment)
};

} // namespace common
} // namespace mmre
