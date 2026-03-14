#pragma once

#include <string>
#include <memory>
#include "SharedMemoryHandle.hpp"

namespace mmre {
namespace memory {

/**
 * @brief Manages POSIX/System V shared memory lifecycle and allocation for large data blobs.
 */
class SharedMemoryManager {
public:
    SharedMemoryManager() = default;
    ~SharedMemoryManager() = default;

    /**
     * @brief Allocates a block of memory from the pre-allocated shared segment.
     * @param size Size in bytes to allocate.
     * @return Handle to the allocated memory.
     */
    common::SharedMemoryHandle Allocate(uint32_t size);

    /**
     * @brief Decrements the reference count of a shared memory block. 
     * Frees it if count reaches zero.
     * @param handle The shared memory handle.
     */
    void Release(const common::SharedMemoryHandle& handle);
    
    /**
     * @brief Retrives the raw pointer in the current process's address space.
     * @param handle The shared memory handle.
     * @return Raw pointer to the data.
     */
    void* GetPointer(const common::SharedMemoryHandle& handle);
};

} // namespace memory
} // namespace mmre
