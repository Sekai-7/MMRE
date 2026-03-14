#pragma once

#include <cstdint>
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include "mmre/common/Types.hpp"
#include "mmre/common/SharedMemoryHandle.hpp"

namespace mmre {
namespace engine_core {

/**
 * @brief Unified representation of any multi-modal data in the cache.
 */
struct UnifiedDataPacket {
    common::TimestampNs timestamp;        ///< Alignment timestamp
    common::ResourceType type;            ///< Resource type
    std::string resourceId;               ///< E.g., "Camera" or "VehicleSignal"
    std::string subResourceId;            ///< E.g., "FrontCamera" or "VehicleSpeed"
    
    // [架构优化说明] 元数据字典，满足 VehicleSignal 的 Groupable 与 Key-Value 存储需求。
    common::MetadataMap metadata;

    // [架构优化说明] 使用 variant 进行多态存储。小体积信号直接存储/字符串直接存系统日志，
    // 大体积图像数据仅存零拷贝共享内存句柄，极大提升内存利用率与传输性能。
    std::variant<std::vector<uint8_t>, common::SharedMemoryHandle, std::string> payload;

    bool isPersisted{false};              ///< State tracking: persisted to UFS
    bool isCheckpoint{false};             ///< State tracking: marked as critical event
    
    // [架构优化说明] 
    // 范围查询的底层双向链表支持。
    // 严重注意：必须使用 weak_ptr 作为 prev 指针，否则循环引用会导致内存泄漏。
    std::weak_ptr<UnifiedDataPacket> prev; 
    std::shared_ptr<UnifiedDataPacket> next;
};

} // namespace engine_core
} // namespace mmre
