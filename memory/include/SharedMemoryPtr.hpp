#pragma once

#include <cstdint>
#include <memory>
#include "SharedMemoryHandle.hpp"

namespace mmre {
namespace memory {

class SharedMemoryManager;

/**
 * @brief RAII wrapper for shared memory handles ensuring zero-leak lifecycle.
 * Fixed: Replaced manual raw pointer ref-counting with std::shared_ptr and std::weak_ptr
 * to eliminate Use-After-Free and Dangling Pointer UB risks.
 */
class SharedMemoryPtr {
public:
    SharedMemoryPtr() = default;

    /**
     * @brief Constructs a new managed SHM pointer, capturing a weak_ptr to the manager.
     */
    SharedMemoryPtr(common::SharedMemoryHandle handle, std::weak_ptr<SharedMemoryManager> manager);

    const common::SharedMemoryHandle& GetHandle() const;

    bool IsValid() const { return handlePtr_ != nullptr; }

private:
    // Holds the handle and manages lifecycle via a custom deleter
    std::shared_ptr<common::SharedMemoryHandle> handlePtr_;
};

} // namespace memory
} // namespace mmre
