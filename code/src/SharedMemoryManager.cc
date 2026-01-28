#include "SharedMemoryManager.h"

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>



SharedMemoryManager::SharedMemoryManager() : fd(-1), memory(nullptr), memorySize(1024 * 1024 * 1024), nextId(0), loc(0) {
    freeMtx.resize(4);
    freeBlocks.resize(4);
    for (int i = 0; i < 4; ++i) {
        freeMtx[i] = std::make_unique<std::mutex>();
        freeBlocks[i] = new MemoryBlock();
    }
    initMemory();
}

SharedMemoryManager::~SharedMemoryManager() {
    for (int i = 0; i < 4; ++i) {
        auto* mb = freeBlocks[i];
        while (mb != nullptr) {
            auto* tmp = mb;
            mb = mb->next;
            delete tmp;
        }
    }
    destroyMemory();
}



SharedMemoryHandle SharedMemoryManager::allocate(size_t size) {
    // std::lock_guard<std::mutex> lock(mutex);
    // void* ptr = allocateActualSharedMemory(size);
    if (!memory) return SharedMemoryHandle();

    size_t align = alignSize(size);
    int idx = sizeToIdx(align);
    auto id = nextId++;
    std::unique_ptr<MemoryBlock> mb = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(*freeMtx[idx]);
        if (freeBlocks[idx]->next != nullptr) {
            mb.reset(freeBlocks[idx]->next);
            freeBlocks[idx]->next = freeBlocks[idx]->next->next;
            mb->refCount = 1;
        }
    }

    std::lock_guard<std::mutex> lock(memoryMtx);
    if (mb == nullptr) {
        void* ptr = static_cast<uint8_t*>(memory) + loc;
        loc += align;
        if (loc >= memorySize) {
            return SharedMemoryHandle();
        }

        mb = std::make_unique<MemoryBlock>(ptr, align, loc - align);
    }

    memoryMap[id] = std::move(mb);
    return SharedMemoryHandle(id, memoryMap[id]->ptr, size);
}

void SharedMemoryManager::deallocate(const int id) {
    MemoryBlock* mb = nullptr;
    {
        std::lock_guard<std::mutex> lock(memoryMtx);
        auto it = memoryMap.find(id);
        if (it == memoryMap.end()) {
            return;
        }

        if (--(it->second->refCount) <= 0) {
        // freeActualSharedMemory(it->second->ptr, it->second->size);
            mb = it->second.release();
            memoryMap.erase(it);
        }
    }

    if (mb != nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(*freeMtx[sizeToIdx(mb->size)]);
        mb->next = freeBlocks[sizeToIdx(mb->size)]->next;
        freeBlocks[sizeToIdx(mb->size)]->next = mb;
    }

    return;
}

void SharedMemoryManager::increaseReferenceCount(const int id) {
    std::lock_guard<std::mutex> lock(memoryMtx);
    auto it = memoryMap.find(id);
    if (it == memoryMap.end()) {
        return;
    }
    it->second->refCount++;
    return;
}

void SharedMemoryManager::initMemory() {
    fd = shm_open("/MMRE", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "[DEFAULT] shm_open failed\n";
    } else {
        ftruncate(fd, memorySize);
        memory = mmap(nullptr, memorySize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (memory == MAP_FAILED) {
            std::cerr << "[DEFAULT] mmap failed\n";
        } else {
            std::cout << "[DEFAULT] mmap success\n";
        }
    }
}

void SharedMemoryManager::destroyMemory() {
    munmap(memory, memorySize);
    close(fd);
    shm_unlink("/MMRE");
    return;
}

size_t SharedMemoryManager::alignSize(size_t size) {
    if (size <= 4 * 1024) {
        return 4 * 1024;
    } else if (size <= 512 * 1024) {
        return 512 * 1024;
    } else if (size <= 4 * 1024 * 1024) {
        return 4 * 1024 * 1024;
    } else if (size <= 12 * 1024 * 1024) {
        return 12 * 1024 * 1024;
    }
    return (size + 4095) & ~4095;
}

int SharedMemoryManager::sizeToIdx(size_t size) {
    if (size == 4 * 1024) {
        return 0;
    } else if (size == 512 * 1024) {
        return 1;
    } else if (size == 4 * 1024 * 1024) {
        return 2;
    } else if (size == 12 * 1024 * 1024) {
        return 3;
    }
    return -1;
}

// ================= SharedMemoryHandle =================

SharedMemoryHandle::SharedMemoryHandle() : id(0), ptr(nullptr), size(0) {}

SharedMemoryHandle::SharedMemoryHandle(uint64_t id, void* ptr, size_t size)
    : id(id), ptr(ptr), size(size) {}

SharedMemoryHandle::SharedMemoryHandle(const SharedMemoryHandle& handle) : id(handle.id), ptr(handle.ptr), size(handle.size), manager(handle.manager) {
    manager->increaseReferenceCount(id);
    return;
}

SharedMemoryHandle& SharedMemoryHandle::operator=(const SharedMemoryHandle& handle) {
    if (this != &handle) {
        id = handle.id;
        ptr = handle.ptr;
        size = handle.size;
        manager = handle.manager;
        manager->increaseReferenceCount(id);
    }
    return *this;
}

SharedMemoryHandle::SharedMemoryHandle(SharedMemoryHandle&& other) noexcept
    : id(other.id), ptr(other.ptr), size(other.size), manager(other.manager) {
    other.id = 0;
    other.ptr = nullptr;
    other.size = 0;
    other.manager = nullptr;
}

SharedMemoryHandle& SharedMemoryHandle::operator=(SharedMemoryHandle&& other) noexcept {
    if (this != &other) {
        id = other.id;
        ptr = other.ptr;
        size = other.size;
        manager = other.manager;
        other.id = 0;
        other.ptr = nullptr;
        other.size = 0;
        other.manager = nullptr;
    }
    return *this;
}

SharedMemoryHandle::~SharedMemoryHandle() {
    if (manager != nullptr) {
        manager->deallocate(id);
        manager = nullptr;
    }
    id = -1;
    ptr = nullptr;
    size = 0;
    return; 
}

bool SharedMemoryHandle::isValid() const { return ptr != nullptr; }
void* SharedMemoryHandle::getPtr() const { return ptr; }
size_t SharedMemoryHandle::getSize() const { return size; }
uint64_t SharedMemoryHandle::getId() const { return id; }