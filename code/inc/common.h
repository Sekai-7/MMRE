#ifndef COMMON_H
#define COMMON_H

#include "SharedMemoryManager.h"

#include <memory>
#include <variant>
// #include <chrono>

using Timestamp = long long;
using Handle = std::variant<std::shared_ptr<void*>, SharedMemoryHandle>;

// 枚举表示数据类型
enum class ResourceType {
    CAMERA,
    AUDIO,
    GPS,
    IMU,
    VEHICLE_SIGNAL,
    UNKNOWN
    // ... 其他数据类型
};

// 统一数据包结构
struct UnifiedDataPacket {
    Timestamp timestamp;
    ResourceType type;
    // void* dataPtr; // 指向共享内存的指针
    Handle dataPtr;
    size_t dataSize;

    UnifiedDataPacket() : timestamp(0), type(ResourceType::CAMERA), dataPtr(), dataSize(0) {} // 添加默认构造函数
    UnifiedDataPacket(Timestamp ts, ResourceType t, Handle ptr, size_t size) : timestamp(ts), type(t), dataPtr(ptr), dataSize(size) {} // 添加构造函数

    UnifiedDataPacket(const UnifiedDataPacket& other) = delete;
    UnifiedDataPacket& operator=(const UnifiedDataPacket& other) = delete;
    // UnifiedDataPacket(const UnifiedDataPacket& other) {
    //     this->timestamp = other.timestamp;
    //     this->type = other.type;
    //     this->dataSize = other.dataSize;
    //     if (std::get_if<SharedMemoryHandle>(&other.dataPtr)) {
            
    //     }

    // }
    // UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {
    //     this->timestamp = other.timestamp;
    //     this->type = other.type;
    //     this->dataSize = other.dataSize;
    //     this->dataPtr = std::move(other.dataPtr);

    // }

    UnifiedDataPacket(UnifiedDataPacket&& other) {
        this->timestamp = other.timestamp;
        this->type = other.type;
        this->dataSize = other.dataSize;
        this->dataPtr = std::move(other.dataPtr);
        return;
    }

    UnifiedDataPacket& operator=(UnifiedDataPacket&& other) {
        if (&other == this)
            return *this;
        this->timestamp = other.timestamp;
        this->type = other.type;
        this->dataSize = other.dataSize;
        this->dataPtr = std::move(other.dataPtr);
        return *this;
    }

    // 拷贝构造函数 (深拷贝共享内存)
    // UnifiedDataPacket(const UnifiedDataPacket& other) : timestamp(other.timestamp), type(other.type), dataSize(other.dataSize) {}

    // 赋值运算符重载 (深拷贝共享内存)
    // UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {}

    // 析构函数 (释放共享内存)
    ~UnifiedDataPacket() {}
};
#endif