#ifndef SHARED_MEMORY_MANAGER_H
#define SHARED_MEMORY_MANAGER_H

#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <memory>

enum class ResourceType : int;

// ================= SharedMemoryHandle =================
class SharedMemoryHandle {
public:
    SharedMemoryHandle();
    SharedMemoryHandle(uint64_t id, void* ptr, size_t size);
    SharedMemoryHandle(SharedMemoryHandle&& other) noexcept;
    SharedMemoryHandle& operator=(SharedMemoryHandle&& other) noexcept;

    // 禁用拷贝
    SharedMemoryHandle(const SharedMemoryHandle&) = delete;
    SharedMemoryHandle& operator=(const SharedMemoryHandle&) = delete;

    bool isValid() const;
    void* getPtr() const;
    size_t getSize() const;
    uint64_t getId() const;

private:
    uint64_t id;
    void* ptr;
    size_t size;
};

// ================= SharedMemoryManager =================
class SharedMemoryManager {
public:
    SharedMemoryManager();
    ~SharedMemoryManager();

    SharedMemoryHandle allocate(size_t size, ResourceType type);
    bool deallocate(const SharedMemoryHandle& handle);
    int getReferenceCount(const SharedMemoryHandle& handle);
    bool increaseReferenceCount(const SharedMemoryHandle& handle);
    SharedMemoryHandle createHandleReference(const SharedMemoryHandle& original);

private:
    std::mutex mutex;
    uint64_t nextId;
    struct MemoryBlock {
        void* ptr;
        size_t size;
        std::atomic<int> refCount;
        ResourceType type;
    };
    std::unordered_map<uint64_t, std::unique_ptr<MemoryBlock>> memoryMap;

    // 平台适配层
    void* allocateActualSharedMemory(size_t size);
    void freeActualSharedMemory(void* ptr, size_t size);
};

#endif // SHARED_MEMORY_MANAGER_H
