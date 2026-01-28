#ifndef SHARED_MEMORY_MANAGER_H
#define SHARED_MEMORY_MANAGER_H

#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <memory>
#include <vector>

// #include "common.h"

enum class ResourceType : int;

class SharedMemoryManager;

// ================= SharedMemoryHandle =================
class SharedMemoryHandle {
public:
    SharedMemoryHandle();
    SharedMemoryHandle(uint64_t id, void* ptr, size_t size);
    SharedMemoryHandle(SharedMemoryHandle&& other) noexcept;
    SharedMemoryHandle& operator=(SharedMemoryHandle&& other) noexcept;

    // 禁用拷贝
    SharedMemoryHandle(const SharedMemoryHandle&);
    SharedMemoryHandle& operator=(const SharedMemoryHandle&);

    ~SharedMemoryHandle();

    bool isValid() const;
    void* getPtr() const;
    size_t getSize() const;
    uint64_t getId() const;

private:
    uint64_t id;
    void* ptr;
    size_t size;
    SharedMemoryManager* manager;
};

// ================= SharedMemoryManager =================
class SharedMemoryManager {
public:
    SharedMemoryManager();
    ~SharedMemoryManager();

    SharedMemoryHandle allocate(size_t size);

    void deallocate(const int);

    void increaseReferenceCount(const int);
private:
    class MemoryBlock {
    public:
        MemoryBlock() : ptr(nullptr), size(0), pos(0), refCount(0) {}
        MemoryBlock(void* ptr, size_t size, size_t pos) : ptr(ptr), size(size), pos(pos), refCount(0) {}
        ~MemoryBlock() {}
        friend class SharedMemoryManager;
    private:
        void* ptr;
        size_t size;
        size_t pos;
        std::atomic<int> refCount;
        MemoryBlock* next;
    };

    // 平台适配
    void initMemory();

    void destroyMemory();

    size_t alignSize(size_t);

    int sizeToIdx(size_t);

private:
    int fd;
    void* memory;
    size_t memorySize;
    std::mutex memoryMtx;
    uint64_t nextId;
    size_t loc;
    std::unordered_map<uint64_t, std::unique_ptr<MemoryBlock>> memoryMap;
    
    std::vector<std::unique_ptr<std::mutex>> freeMtx;
    std::vector<MemoryBlock*> freeBlocks;

};

#endif // SHARED_MEMORY_MANAGER_H
