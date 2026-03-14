#pragma once

#include <cstdint>
#include <atomic>

namespace mmre {
namespace memory {

/**
 * @brief Plain Old Data (POD) struct for shared memory cross-process exchange.
 * 仅用于通过 IPC 发送。
 */
struct SharedMemoryHandle {
    uint32_t poolId;
    uint32_t blockId;
    uint64_t offset;
    uint32_t size;
};

// Forward declaration of Manager to handle release
class SharedMemoryManager;

/**
 * @brief RAII wrapper for shared memory handles ensuring zero-leak lifecycle.
 * 在引用计数降为 0 时自动通知大页内存池回收块。
 */
class SharedMemoryPtr {
public:
    SharedMemoryPtr(SharedMemoryHandle handle, SharedMemoryManager* manager);
    ~SharedMemoryPtr();

    // Copy semantics (increments ref count atomically)
    SharedMemoryPtr(const SharedMemoryPtr& other);
    SharedMemoryPtr& operator=(const SharedMemoryPtr& other);

    // Move semantics (transfers ownership without altering ref count)
    SharedMemoryPtr(SharedMemoryPtr&& other) noexcept;
    SharedMemoryPtr& operator=(SharedMemoryPtr&& other) noexcept;

    const SharedMemoryHandle& GetHandle() const { return handle_; }
    void* GetRawPointer() const; 

private:
    SharedMemoryHandle handle_;
    SharedMemoryManager* manager_{nullptr};
    
    void AddRef();
    void ReleaseRef();
};

} // namespace memory
} // namespace mmre
