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

static int ionFd = -1;

void* SharedMemoryManager::allocateActualSharedMemory(size_t size) {
    if (ionFd < 0) {
        ionFd = open("/dev/ion", O_RDWR);
        if (ionFd < 0) {
            std::cerr << "[QCOM] Failed to open /dev/ion, fallback to malloc\n";
            return malloc(size);
        }
    }

    struct ion_allocation_data alloc {};
    alloc.len = size;
    alloc.align = 4096;
    alloc.heap_id_mask = 1 << ION_SYSTEM_HEAP_ID;   // 选 system heap
    alloc.flags = 0;

    if (ioctl(ionFd, ION_IOC_ALLOC, &alloc) < 0) {
        std::cerr << "[QCOM] ION_IOC_ALLOC failed, fallback to malloc\n";
        return malloc(size);
    }

    struct ion_fd_data fdData {};
    fdData.handle = alloc.handle;

    if (ioctl(ionFd, ION_IOC_SHARE, &fdData) < 0) {
        std::cerr << "[QCOM] ION_IOC_SHARE failed, fallback to malloc\n";
        return malloc(size);
    }

    void* vaddr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fdData.fd, 0);
    if (vaddr == MAP_FAILED) {
        std::cerr << "[QCOM] mmap failed, fallback to malloc\n";
        return malloc(size);
    }

    std::cerr << "[QCOM] Allocated ION buffer, size=" << size << std::endl;
    return vaddr;
}

void SharedMemoryManager::freeActualSharedMemory(void* ptr, size_t size) {
    if (!ptr) return;
    if (munmap(ptr, size) != 0) {
        free(ptr);
    }
}


#elif defined(NVIDIA)
#include <nvscibuf.h>
#include <cstring>

void* SharedMemoryManager::allocateActualSharedMemory(size_t size) {
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

void SharedMemoryManager::freeActualSharedMemory(void* ptr, size_t size) {
    if (!ptr) return;
    // NOTE: NvSciBuf 的释放通常需要调用 NvSciBufObjFree
    // 这里示意：如果不是 NvSci 分配的，就 free
    free(ptr);
}


#else
#include <sys/mman.h>
#include <unistd.h>

void* SharedMemoryManager::allocateActualSharedMemory(size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "[DEFAULT] mmap failed, fallback to malloc\n";
        return malloc(size);
    }
    std::cerr << "[DEFAULT] mmap allocated\n";
    return ptr;
}

void SharedMemoryManager::freeActualSharedMemory(void* ptr, size_t size) {
    if (!ptr) return;
    if (munmap(ptr, size) != 0) {
        free(ptr);
    }
}
#endif

// ================= 公共逻辑实现 =================
SharedMemoryManager::SharedMemoryManager() : nextId(1) {}

SharedMemoryManager::~SharedMemoryManager() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& pair : memoryMap) {
        freeActualSharedMemory(pair.second->ptr, pair.second->size);
    }
}

SharedMemoryHandle SharedMemoryManager::allocate(size_t size, ResourceType type) {
    std::lock_guard<std::mutex> lock(mutex);
    void* ptr = allocateActualSharedMemory(size);
    if (!ptr) return SharedMemoryHandle();

    uint64_t id = nextId++;
    auto block = std::make_unique<MemoryBlock>();
    block->ptr = ptr;
    block->size = size;
    block->refCount = 1;
    block->type = type;

    memoryMap[id] = std::move(block);
    return SharedMemoryHandle(id, ptr, size);
}

bool SharedMemoryManager::deallocate(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = memoryMap.find(handle.getId());
    if (it == memoryMap.end()) return false;

    if (--(it->second->refCount) <= 0) {
        freeActualSharedMemory(it->second->ptr, it->second->size);
        memoryMap.erase(it);
    }
    return true;
}

int SharedMemoryManager::getReferenceCount(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = memoryMap.find(handle.getId());
    if (it == memoryMap.end()) return 0;
    return it->second->refCount.load();
}

bool SharedMemoryManager::increaseReferenceCount(const SharedMemoryHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = memoryMap.find(handle.getId());
    if (it == memoryMap.end()) return false;
    it->second->refCount++;
    return true;
}

SharedMemoryHandle SharedMemoryManager::createHandleReference(const SharedMemoryHandle& original) {
    if (!increaseReferenceCount(original)) return SharedMemoryHandle();

    std::lock_guard<std::mutex> lock(mutex);
    auto it = memoryMap.find(original.getId());
    if (it == memoryMap.end()) return SharedMemoryHandle();
    return SharedMemoryHandle(it->first, it->second->ptr, it->second->size);
}

// ================= SharedMemoryHandle =================
SharedMemoryHandle::SharedMemoryHandle() : id(0), ptr(nullptr), size(0) {}

SharedMemoryHandle::SharedMemoryHandle(uint64_t id, void* ptr, size_t size)
    : id(id), ptr(ptr), size(size) {}

SharedMemoryHandle::SharedMemoryHandle(SharedMemoryHandle&& other) noexcept
    : id(other.id), ptr(other.ptr), size(other.size) {
    other.id = 0;
    other.ptr = nullptr;
    other.size = 0;
}

SharedMemoryHandle& SharedMemoryHandle::operator=(SharedMemoryHandle&& other) noexcept {
    if (this != &other) {
        id = other.id;
        ptr = other.ptr;
        size = other.size;
        other.id = 0;
        other.ptr = nullptr;
        other.size = 0;
    }
    return *this;
}

bool SharedMemoryHandle::isValid() const { return ptr != nullptr; }
void* SharedMemoryHandle::getPtr() const { return ptr; }
size_t SharedMemoryHandle::getSize() const { return size; }
uint64_t SharedMemoryHandle::getId() const { return id; }