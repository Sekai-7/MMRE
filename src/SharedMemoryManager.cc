#include "SharedMemoryManager.h"
#include <iostream>
#include <cstdlib>
#include <utility>

#ifdef QCOM
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ion.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

static int ion_fd = -1;

void* SharedMemoryManager::allocate_actual_shared_memory(size_t size) {
    if (ion_fd < 0) {
        ion_fd = open("/dev/ion", O_RDWR);
        if (ion_fd < 0) {
            std::cerr << "[QCOM] Failed to open /dev/ion, fallback to malloc\n";
            return malloc(size);
        }
    }

    struct ion_allocation_data alloc {};
    alloc.len = size;
    alloc.align = 4096;
    alloc.heap_id_mask = 1 << ION_SYSTEM_HEAP_ID;   // 选 system heap
    alloc.flags = 0;

    if (ioctl(ion_fd, ION_IOC_ALLOC, &alloc) < 0) {
        std::cerr << "[QCOM] ION_IOC_ALLOC failed, fallback to malloc\n";
        return malloc(size);
    }

    struct ion_fd_data fd_data {};
    fd_data.handle = alloc.handle;

    if (ioctl(ion_fd, ION_IOC_SHARE, &fd_data) < 0) {
        std::cerr << "[QCOM] ION_IOC_SHARE failed, fallback to malloc\n";
        return malloc(size);
    }

    void* vaddr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data.fd, 0);
    if (vaddr == MAP_FAILED) {
        std::cerr << "[QCOM] mmap failed, fallback to malloc\n";
        return malloc(size);
    }

    std::cerr << "[QCOM] Allocated ION buffer, size=" << size << std::endl;
    return vaddr;
}

void SharedMemoryManager::free_actual_shared_memory(void* ptr, size_t size) {
    if (!ptr) return;
    if (munmap(ptr, size) != 0) {
        free(ptr);
    }
}


#elif defined(NVIDIA)
#include <nvscibuf.h>
#include <cstring>

void* SharedMemoryManager::allocate_actual_shared_memory(size_t size) {
    NvSciBufAttrList attrList;
    NvSciBufObj bufObj;

    // 创建属性列表
    NvSciError err = NvSciBufAttrListCreate(&attrList);
    if (err != NvSciError_Success) {
        std::cerr << "[NVIDIA] NvSciBufAttrListCreate failed, fallback to malloc\n";
        return malloc(size);
    }

    NvSciBufAttrKeyValuePair attrs[] = {
        { NvSciBufGeneralAttrKey_Types, (void*)&(NvSciBufType_RawBuffer), sizeof(NvSciBufType) },
        { NvSciBufGeneralAttrKey_Size,  (void*)&size, sizeof(size_t) }
    };

    err = NvSciBufAttrListSetAttrs(attrList, attrs, 2);
    if (err != NvSciError_Success) {
        std::cerr << "[NVIDIA] NvSciBufAttrListSetAttrs failed, fallback to malloc\n";
        return malloc(size);
    }

    err = NvSciBufObjCreate(attrList, &bufObj);
    if (err != NvSciError_Success) {
        std::cerr << "[NVIDIA] NvSciBufObjCreate failed, fallback to malloc\n";
        return malloc(size);
    }

    void* ptr = nullptr;
    size_t actualSize = 0;
    err = NvSciBufObjGetCpuPtr(bufObj, &ptr, &actualSize);
    if (err != NvSciError_Success || !ptr) {
        std::cerr << "[NVIDIA] NvSciBufObjGetCpuPtr failed, fallback to malloc\n";
        return malloc(size);
    }

    std::cerr << "[NVIDIA] Allocated NvSciBuf, size=" << actualSize << std::endl;
    return ptr;
}

void SharedMemoryManager::free_actual_shared_memory(void* ptr, size_t size) {
    if (!ptr) return;
    // NOTE: NvSciBuf 的释放通常需要调用 NvSciBufObjFree
    // 这里示意：如果不是 NvSci 分配的，就 free
    free(ptr);
}


#else
#include <sys/mman.h>
#include <unistd.h>

void* SharedMemoryManager::allocate_actual_shared_memory(size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "[DEFAULT] mmap failed, fallback to malloc\n";
        return malloc(size);
    }
    std::cerr << "[DEFAULT] mmap allocated\n";
    return ptr;
}

void SharedMemoryManager::free_actual_shared_memory(void* ptr, size_t size) {
    if (!ptr) return;
    if (munmap(ptr, size) != 0) {
        free(ptr);
    }
}
#endif

// ================= 公共逻辑实现 =================
SharedMemoryManager::SharedMemoryManager() : next_id_(1) {}

SharedMemoryManager::~SharedMemoryManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : memory_map_) {
        free_actual_shared_memory(pair.second->ptr, pair.second->size);
    }
}

SharedMemoryHandle SharedMemoryManager::allocate(size_t size, ResourceType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    void* ptr = allocate_actual_shared_memory(size);
    if (!ptr) return SharedMemoryHandle();

    uint64_t id = next_id_++;
    auto block = std::make_unique<MemoryBlock>();
    block->ptr = ptr;
    block->size = size;
    block->ref_count = 1;
    block->type = type;

    memory_map_[id] = std::move(block);
    return SharedMemoryHandle(id, ptr, size);
}

bool SharedMemoryManager::deallocate(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memory_map_.find(handle.get_id());
    if (it == memory_map_.end()) return false;

    if (--(it->second->ref_count) <= 0) {
        free_actual_shared_memory(it->second->ptr, it->second->size);
        memory_map_.erase(it);
    }
    return true;
}

int SharedMemoryManager::get_reference_count(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memory_map_.find(handle.get_id());
    if (it == memory_map_.end()) return 0;
    return it->second->ref_count.load();
}

bool SharedMemoryManager::increase_reference_count(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memory_map_.find(handle.get_id());
    if (it == memory_map_.end()) return false;
    it->second->ref_count++;
    return true;
}

SharedMemoryHandle SharedMemoryManager::create_handle_reference(const SharedMemoryHandle& original) {
    if (!increase_reference_count(original)) return SharedMemoryHandle();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memory_map_.find(original.get_id());
    if (it == memory_map_.end()) return SharedMemoryHandle();
    return SharedMemoryHandle(it->first, it->second->ptr, it->second->size);
}

// ================= SharedMemoryHandle =================
SharedMemoryHandle::SharedMemoryHandle() : id_(0), ptr_(nullptr), size_(0) {}

SharedMemoryHandle::SharedMemoryHandle(uint64_t id, void* ptr, size_t size)
    : id_(id), ptr_(ptr), size_(size) {}

SharedMemoryHandle::SharedMemoryHandle(SharedMemoryHandle&& other) noexcept
    : id_(other.id_), ptr_(other.ptr_), size_(other.size_) {
    other.id_ = 0;
    other.ptr_ = nullptr;
    other.size_ = 0;
}

SharedMemoryHandle& SharedMemoryHandle::operator=(SharedMemoryHandle&& other) noexcept {
    if (this != &other) {
        id_ = other.id_;
        ptr_ = other.ptr_;
        size_ = other.size_;
        other.id_ = 0;
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

bool SharedMemoryHandle::is_valid() const { return ptr_ != nullptr; }
void* SharedMemoryHandle::get_ptr() const { return ptr_; }
size_t SharedMemoryHandle::get_size() const { return size_; }
uint64_t SharedMemoryHandle::get_id() const { return id_; }
