// sekai-7/mmre/MMRE-ai/memory/src/SharedMemoryPtr.cpp
#include "SharedMemoryPtr.hpp"

namespace mmre {
namespace memory {

SharedMemoryPtr::SharedMemoryPtr(common::SharedMemoryHandle handle, std::shared_ptr<void> mappedMemory)
    : handle_(std::move(handle)), mappedMemory_(std::move(mappedMemory)) {
}

const common::SharedMemoryHandle& SharedMemoryPtr::GetHandle() const {
    return handle_;
}

} // namespace memory
} // namespace mmre