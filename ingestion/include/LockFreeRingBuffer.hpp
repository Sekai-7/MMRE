#pragma once

#include <atomic>
#include <cstddef>

namespace mmre {
namespace ingestion {

/**
 * @brief Single-Producer Single-Consumer (SPSC) Lock-free Ring Buffer.
 * 
 * // [架构优化说明]
 * // 针对高频突发流量（如 100+ 路 CAN 信号同时到达），在 Reactor 的 IO 线程与工作线程间传递数据。
 * // 使用 C++11 std::atomic 内存屏障替代传统的 std::mutex/std::condition_variable。
 * // 彻底消除锁竞争导致的上下文切换与队头阻塞波峰，保证吞吐量与极低延迟。
 */
template <typename T, size_t Capacity>
class LockFreeRingBuffer {
public:
    LockFreeRingBuffer() : head_(0), tail_(0) {}

    bool Push(const T& item) {
        size_t currentTail = tail_.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) % Capacity;
        
        if (nextTail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer is full
        }
        
        buffer_[currentTail] = item;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    bool Pop(T& item) {
        size_t currentHead = head_.load(std::memory_order_relaxed);
        
        if (currentHead == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer is empty
        }
        
        item = buffer_[currentHead];
        head_.store((currentHead + 1) % Capacity, std::memory_order_release);
        return true;
    }

private:
    T buffer_[Capacity];
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

} // namespace ingestion
} // namespace mmre
