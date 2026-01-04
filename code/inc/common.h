#ifndef COMMON_H
#define COMMON_H

// #include "SharedMemoryManager.h"

#include <memory>
#include <variant>
// #include <chrono>

using Timestamp = long long;
// using Handle = std::variant<void*, SharedMemoryHandle>;

// 枚举表示数据类型
enum class ResourceType {
    CAMERA,
    AUDIO,
    GPS,
    IMU,
    VEHICLE_SIGNAL
    // ... 其他数据类型
};

// 统一数据包结构
struct UnifiedDataPacket {
    Timestamp timestamp;
    ResourceType type;
    void* dataPtr; // 指向共享内存的指针
    // Handle dataPtr;
    size_t dataSize;

    void* getPtr() {
        // struct Visitor {
        //     void* operator() (void* ptr) {
        //         return ptr; 
        //     }
        //     void* operator() (SharedMemoryHandle& handle) {
        //         if (handle.isValid() == false || handle.getSize() == 0)
        //             return nullptr;
        //         return handle.getPtr();
        //     }
        // };
        // return std::visit(Visitor{}, dataPtr);
        return dataPtr;
    }

    const void* getPtr() const {
        // struct Visitor {
        //     void* operator() (void* ptr) const {
        //         return ptr; 
        //     }
        //     void* operator() (const SharedMemoryHandle& handle) const {
        //         if (handle.isValid() == false || handle.getSize() == 0)
        //             return nullptr;
        //         return handle.getPtr();
        //     }
        // };
        // return std::visit(Visitor{}, dataPtr);
        return dataPtr;
    }

    UnifiedDataPacket() : timestamp(0), type(ResourceType::CAMERA), dataPtr(nullptr), dataSize(0) {} // 添加默认构造函数
    UnifiedDataPacket(Timestamp ts, ResourceType t, void* ptr, size_t size) : timestamp(ts), type(t), dataPtr(ptr), dataSize(size) {} // 添加构造函数

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