// Code by LIU Lucia

#include "SharedMemoryManager.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <chrono>

// 模拟摄像头数据生产者
void camera_producer(SharedMemoryManager& manager, SharedMemoryHandle handle) {
    if (!handle.is_valid()) return;

    char* buf = static_cast<char*>(handle.get_ptr());
    strcpy(buf, "Frame from camera");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[Camera Producer] Wrote: " << buf << std::endl;

    manager.deallocate(handle);  // 引用 -1
}

// 模拟摄像头数据消费者
void camera_consumer(SharedMemoryManager& manager, SharedMemoryHandle handle) {
    if (!handle.is_valid()) return;

    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 等待生产完成
    char* buf = static_cast<char*>(handle.get_ptr());

    std::cout << "[Camera Consumer] Read: " << buf << std::endl;

    manager.deallocate(handle);  // 引用 -1
}

int main() {
    SharedMemoryManager manager;

    // 分配一块共享内存，模拟一帧摄像头数据
    auto handle = manager.allocate(4096, ResourceType::TYPE_A);
    if (!handle.is_valid()) {
        std::cerr << "Failed to allocate shared memory!" << std::endl;
        return 1;
    }

    std::cout << "[Main] Allocated id=" << handle.get_id()
              << ", size=" << handle.get_size() << std::endl;

    // 创建两个引用（生产者、消费者）
    auto handle_producer = manager.create_handle_reference(handle);
    auto handle_consumer = manager.create_handle_reference(handle);

    std::cout << "[Main] Ref count after clone = "
              << manager.get_reference_count(handle) << std::endl;

    // 启动生产者和消费者线程
    std::thread t1(camera_producer, std::ref(manager), std::move(handle_producer));
    std::thread t2(camera_consumer, std::ref(manager), std::move(handle_consumer));

    // 主线程释放自己的引用
    manager.deallocate(handle);

    t1.join();
    t2.join();

    std::cout << "[Main] Test finished. All refs should be released." << std::endl;

    return 0;
}
