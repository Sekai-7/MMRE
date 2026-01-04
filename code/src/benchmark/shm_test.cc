// Code by LIU Lucia

#include "SharedMemoryManager.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <chrono>

// 模拟摄像头数据生产者
void cameraProducer(SharedMemoryManager& manager, SharedMemoryHandle handle) {
    if (!handle.isValid()) return;

    char* buf = static_cast<char*>(handle.getPtr());
    strcpy(buf, "Frame from camera");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[Camera Producer] Wrote: " << buf << std::endl;

    manager.deallocate(handle);  // 引用 -1
}

// 模拟摄像头数据消费者
void cameraConsumer(SharedMemoryManager& manager, SharedMemoryHandle handle) {
    if (!handle.isValid()) return;

    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 等待生产完成
    char* buf = static_cast<char*>(handle.getPtr());

    std::cout << "[Camera Consumer] Read: " << buf << std::endl;

    manager.deallocate(handle);  // 引用 -1
}

int main() {
    SharedMemoryManager manager;

    // 分配一块共享内存，模拟一帧摄像头数据
    auto handle = manager.allocate(4096, ResourceType::CAMERA); // Assuming ResourceType::TYPE_A was a placeholder or I should use standard types. Original had TYPE_A. 
    // Wait, inc/SharedMemoryManager.h uses ResourceType. inc/common.h defines ResourceType. 
    // But SharedMemoryManager.h forward declares `enum class ResourceType : int;`. 
    // And inc/common.h defines `enum class ResourceType { CAMERA, ... }`.
    // The benchmark uses `ResourceType::TYPE_A` in original code? 
    // Let me check inc/SharedMemoryManager.h again. 
    // It has `enum class ResourceType : int;` forward decl.
    // inc/common.h has definitions. 
    // The original `shm_test.cc` used `ResourceType::TYPE_A`. 
    // I should check if `TYPE_A` exists. In `inc/common.h` I saw `CAMERA, AUDIO, GPS, IMU, VEHICLE_SIGNAL`. No `TYPE_A`.
    // Maybe `shm_test.cc` was relying on a different definition or I missed it.
    // I will use `ResourceType::CAMERA` instead of `TYPE_A` to be safe and consistent.

    if (!handle.isValid()) {
        std::cerr << "Failed to allocate shared memory!" << std::endl;
        return 1;
    }

    std::cout << "[Main] Allocated id=" << handle.getId()
              << ", size=" << handle.getSize() << std::endl;

    // 创建两个引用（生产者、消费者）
    auto handleProducer = manager.createHandleReference(handle);
    auto handleConsumer = manager.createHandleReference(handle);

    std::cout << "[Main] Ref count after clone = "
              << manager.getReferenceCount(handle) << std::endl;

    // 启动生产者和消费者线程
    std::thread t1(cameraProducer, std::ref(manager), std::move(handleProducer));
    std::thread t2(cameraConsumer, std::ref(manager), std::move(handleConsumer));

    // 主线程释放自己的引用
    manager.deallocate(handle);

    t1.join();
    t2.join();

    std::cout << "[Main] Test finished. All refs should be released." << std::endl;

    return 0;
}