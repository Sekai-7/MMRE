#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mmre {
namespace query {

/**
 * @brief 纯逻辑的语法解析器。
 * 不再依赖服务端的任何执行引擎实体，专注于在客户端侧将外部协议编译为内部字节码。
 */
class InterfaceParser {
public:
    /**
     * @brief Parses an SQL-like query string into strict binary bytecode.
     * @return 编译后的字节码 (Bytecode)，直接用于 IPC 传输。
     */
    static std::vector<uint8_t> CompileSqlToBytecode(const std::string& sqlQuery);
};

} // namespace query
} // namespace mmre
