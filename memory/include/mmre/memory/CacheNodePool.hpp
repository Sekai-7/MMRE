#pragma once

#include <memory>
#include <mutex>
// Forward declaration
namespace mmre { namespace engine_core { struct UnifiedDataPacket; } }

namespace mmre {
namespace memory {

/**
 * @brief Object pool for caching nodes to prevent memory fragmentation.
 * 
 * // [架构优化说明]
 * // 嵌入式系统长时间运行下，频繁的 new/delete 会产生严重内存碎片。
 * // 采用 Slab Allocation 原理预分配定长数据包节点，消除高频离散信号（如100Hz CAN）
 * // 带来的 malloc 系统调用开销，实现 <1ms 的数据注入延迟。
 */
class CacheNodePool {
public:
    static CacheNodePool& GetInstance();

    /**
     * @brief Allocate a data packet object from the pool.
     * @return std::shared_ptr to the packet with a custom deleter returning it to the pool.
     */
    std::shared_ptr<engine_core::UnifiedDataPacket> AllocatePacket();

private:
    CacheNodePool() = default;
    ~CacheNodePool() = default;

    // TODO: Internal lock-free or lightweight locked pool implementation
};

} // namespace memory
} // namespace mmre
