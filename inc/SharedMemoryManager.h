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

    bool is_valid() const;
    void* get_ptr() const;
    size_t get_size() const;
    uint64_t get_id() const;

private:
    uint64_t id_;
    void* ptr_;
    size_t size_;
};

// ================= SharedMemoryManager =================
class SharedMemoryManager {
public:
    SharedMemoryManager();
    ~SharedMemoryManager();

    SharedMemoryHandle allocate(size_t size, ResourceType type);
    bool deallocate(const SharedMemoryHandle& handle);
    int get_reference_count(const SharedMemoryHandle& handle);
    bool increase_reference_count(const SharedMemoryHandle& handle);
    SharedMemoryHandle create_handle_reference(const SharedMemoryHandle& original);

private:
    std::mutex mutex_;
    uint64_t next_id_;
    struct MemoryBlock {
        void* ptr;
        size_t size;
        std::atomic<int> ref_count;
        ResourceType type;
    };
    std::unordered_map<uint64_t, std::unique_ptr<MemoryBlock>> memory_map_;

    // 平台适配层
    void* allocate_actual_shared_memory(size_t size);
    void free_actual_shared_memory(void* ptr, size_t size);
};

#endif // SHARED_MEMORY_MANAGER_H
