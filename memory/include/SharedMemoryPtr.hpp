
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
     * @brief Const-correctness 修复：防止只读快照被意外修改。
     */
    const void* GetRawData() const { return mappedMemory_.get(); }
    
    /**
     * @brief 获取可变指针（仅限于构建/反序列化阶段使用）。
     */
    void* GetRawData() { return mappedMemory_.get(); }
    
    /**
     * @brief Convenience operator to cast data directly to required struct/type.
     */
    template<typename T>
    const T* As() const { return static_cast<const T*>(GetRawData()); }

    template<typename T>
    T* As() { return static_cast<T*>(GetRawData()); }

    bool IsValid() const { return mappedMemory_ != nullptr; }

private:
    common::SharedMemoryHandle handle_;
    // RAII shared ownership of the mapped memory (Deleter resolves back to SharedMemoryManager)
    std::shared_ptr<void> mappedMemory_;
};

} // namespace memory
} // namespace mmre