// sekai-7/mmre/MMRE-ai/memory/src/SharedMemoryManager.cpp
#include "SharedMemoryManager.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <uuid/uuid.h> // 需要在 CMake 中链接 libuuid

namespace mmre {
namespace memory {

common::SharedMemoryHandle SharedMemoryManager::Allocate(uint32_t size) {
    common::SharedMemoryHandle handle;
    handle.size = size;
    
    // 生成全局唯一的 SHM 标识符
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    handle.name = std::string("/mmre_shm_") + uuid_str;

    // 创建并截断共享内存对象
    int fd = shm_open(handle.name.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to create shared memory: " + handle.name);
    }
    
    if (ftruncate(fd, size) == -1) {
        close(fd);
        shm_unlink(handle.name.c_str());
        throw std::runtime_error("Failed to set size for shared memory.");
    }
    close(fd);
    
    return handle;
}

std::shared_ptr<void> SharedMemoryManager::GetPointer(const common::SharedMemoryHandle& handle) {
    int fd = shm_open(handle.name.c_str(), O_RDWR, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to open shared memory for mapping.");
    }

    void* mappedPtr = mmap(nullptr, handle.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); // 映射完成后可以关闭 fd

    if (mappedPtr == MAP_FAILED) {
        throw std::runtime_error("Failed to mmap shared memory.");
    }

    // 利用 std::shared_ptr 自定义删除器实现严格的 RAII 生命周期控制
    auto deleter = [this, handle](void* ptr) {
        munmap(ptr, handle.size);
        this->Release(handle);
    };

    return std::shared_ptr<void>(mappedPtr, deleter);
}

void SharedMemoryManager::Release(const common::SharedMemoryHandle& handle) {
    // 引用计数归零后，清理操作系统层的共享内存文件
    shm_unlink(handle.name.c_str());
}

} // namespace memory
} // namespace mmre