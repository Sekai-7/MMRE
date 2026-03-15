#pragma once

#include <vector>
#include <cstdint>
#include "Types.hpp"

namespace mmre {
namespace common {

/**
 * @brief 跨进程共享的标准化查询请求 DTO。
 * 剥离了所有对核心引擎组件的依赖，允许 Client 与 Server 安全共享。
 */
struct QueryRequest {
    uint32_t resourceIdHash;
    uint32_t subResourceIdHash;
    TimestampNs startTime;
    TimestampNs endTime; 
    bool isLatest{false};        

    // 预编译的过滤字节码，彻底消除服务端的字符串解析 CPU 开销
    std::vector<uint8_t> compiledFilterBytecode{};
};

struct CursorResponse {
    uint64_t cursorId{0};          
    uint32_t totalEstimatedCount{0}; 
};

} // namespace common
} // namespace mmre