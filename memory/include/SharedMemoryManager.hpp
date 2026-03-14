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
     * @brief Retrives the raw pointer mapped in the current process's address space,
     * wrapped in a C++11 std::shared_ptr with a custom deleter. 
     * This enforces strict RAII, completely eliminating the risk of SHM leaks.
     * @param handle The shared memory handle.
     * @return RAII wrapped pointer to the data.
     */
    std::shared_ptr<void> GetPointer(const common::SharedMemoryHandle& handle);

private:
    /**
     * @brief Internal method used by custom deleter to decrement ref count.
     */
    void Release(const common::SharedMemoryHandle& handle);
};

} // namespace memory
} // namespace mmre
