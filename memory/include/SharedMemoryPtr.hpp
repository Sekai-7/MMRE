
#pragma once

#include <cstdint>
#include <memory>
#include "SharedMemoryHandle.hpp"

namespace mmre {
namespace memory {

class SharedMemoryManager;

/**
 * @brief RAII wrapper for shared memory handles ensuring zero-leak lifecycle.
 * Fixed: Exposes actual mapped void* data pointer to resolve the logic gap 
 * where consumers could not access zero-copy data for image/audio decoding.
 */
class SharedMemoryPtr {
public:
    SharedMemoryPtr() = default;

    /**
     * @brief Constructs a new managed SHM pointer, capturing the mapped memory
     * block via std::shared_ptr to ensure safe lifecycle management across copies.
     */
    SharedMemoryPtr(common::SharedMemoryHandle handle, std::shared_ptr<void> mappedMemory);

    /**
     * @brief Returns the IPC handle to transmit this memory to other processes.
     */
    const common::SharedMemoryHandle& GetHandle() const;

    /**
     * @brief Grants read/write access to the actual zero-copy payload in current process space.
     * Guaranteed to be valid as long as this object is alive.
     */
    void* GetRawData() const { return mappedMemory_.get(); }
    
    /**
     * @brief Convenience operator to cast data directly to required struct/type.
     */
    template<typename T>
    T* As() const { return static_cast<T*>(GetRawData()); }

    bool IsValid() const { return mappedMemory_ != nullptr; }

private:
    common::SharedMemoryHandle handle_;
    // RAII shared ownership of the mapped memory (Deleter resolves back to SharedMemoryManager)
    std::shared_ptr<void> mappedMemory_;
};

} // namespace memory
} // namespace mmre