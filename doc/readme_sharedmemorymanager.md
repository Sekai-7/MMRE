g++ -std=c++17 -Wall -O2 SharedMemoryManager.cc SharedMemoryHandle.cc main.cc -o shm_test -pthread

## 测试main.cc覆盖：
1. 多引用计数
- main、camera_producer、camera_consumer 三个线程都拿到同一个 handle 的引用，计数应该是 3。
- 每个线程释放时，计数自动减少。最后一个释放后内存销毁。

2. 多线程并发访问
- camera_producer 写数据 → camera_consumer 读数据 。
- 模拟摄像头、音频模块共享数据的场景。

3. 自动回收
- 所有 deallocate() 调用完成后，引用计数降到 0，内存被 free_actual_shared_memory 正确释放。



---

##  **SharedMemoryManager 接口**

| 方法                                                                               | 参数                            | 返回值                  | 功能说明                        |
| -------------------------------------------------------------------------------- | ----------------------------- | -------------------- | --------------------------- |
| `SharedMemoryManager()`                                                          | -                             | 构造函数                 | 初始化内存管理器，`next_id_` 从 1 开始。 |
| `~SharedMemoryManager()`                                                         | -                             | 析构函数                 | 析构时释放所有仍然存在的内存块。            |
| `SharedMemoryHandle allocate(size_t size, ResourceType type)`                    | `size`: 分配字节数<br>`type`: 资源类型 | `SharedMemoryHandle` | 分配一块共享内存，创建内存块并返回句柄。        |
| `bool deallocate(const SharedMemoryHandle& handle)`                              | `handle`: 共享内存句柄              | `true/false`         | 引用计数减 1；如果为 0，则释放内存并移除。     |
| `int get_reference_count(const SharedMemoryHandle& handle)`                      | `handle`: 共享内存句柄              | `int` (引用计数)         | 获取当前句柄的引用计数。                |
| `bool increase_reference_count(const SharedMemoryHandle& handle)`                | `handle`: 共享内存句柄              | `true/false`         | 引用计数 +1。                    |
| `SharedMemoryHandle create_handle_reference(const SharedMemoryHandle& original)` | `original`: 原始句柄              | `SharedMemoryHandle` | 基于已有句柄创建一个新的引用句柄（ref+1）。    |

---

##  **SharedMemoryManager (平台相关实现)**

| 平台宏      | 方法                                                  | 功能说明                                      |
| -------- | --------------------------------------------------- | ----------------------------------------- |
| `QCOM`   | `allocate_actual_shared_memory(size_t size)`        | 使用 **ION** 分配共享内存，如果失败则回退到 `malloc`。      |
|          | `free_actual_shared_memory(void* ptr, size_t size)` | `munmap` 释放；如果失败则 `free`。                 |
| `NVIDIA` | `allocate_actual_shared_memory(size_t size)`        | 使用 **NvSciBuf** 分配共享内存，如果失败则回退到 `malloc`。 |
|          | `free_actual_shared_memory(void* ptr, size_t size)` | 理论上需要调用 `NvSciBufObjFree`；此处简化为 `free`。   |
| 其他       | `allocate_actual_shared_memory(size_t size)`        | 使用 `mmap` 分配匿名共享内存，失败时回退到 `malloc`。       |
|          | `free_actual_shared_memory(void* ptr, size_t size)` | 使用 `munmap` 释放；失败则 `free`。                |

---

##  **SharedMemoryHandle 接口**

| 方法                                                                   | 参数                                | 返回值        | 功能说明             |
| -------------------------------------------------------------------- | --------------------------------- | ---------- | ---------------- |
| `SharedMemoryHandle()`                                               | -                                 | 默认构造函数     | 创建一个无效句柄。        |
| `SharedMemoryHandle(uint64_t id, void* ptr, size_t size)`            | id: 内存块 ID<br>ptr: 指针<br>size: 大小 | 构造函数       | 创建一个有效句柄。        |
| `SharedMemoryHandle(SharedMemoryHandle&& other) noexcept`            | other: 右值引用                       | 移动构造函数     | 接管资源并清空源对象。      |
| `SharedMemoryHandle& operator=(SharedMemoryHandle&& other) noexcept` | other: 右值引用                       | 自身引用       | 移动赋值，接管资源并清空源对象。 |
| `bool is_valid() const`                                              | -                                 | true/false | 是否为有效句柄（指针非空）。   |
| `void* get_ptr() const`                                              | -                                 | 指针         | 获取内存地址。          |
| `size_t get_size() const`                                            | -                                 | 字节数        | 获取内存大小。          |
| `uint64_t get_id() const`                                            | -                                 | ID         | 获取分配时生成的唯一 ID。   |

---

##  **内部数据结构**

### `MemoryBlock`

* `void* ptr` → 内存指针
* `size_t size` → 内存大小
* `std::atomic<int> ref_count` → 引用计数
* `ResourceType type` → 资源类型（枚举）

### `SharedMemoryManager` 私有成员

* `std::unordered_map<uint64_t, std::unique_ptr<MemoryBlock>> memory_map_` → ID 到内存块的映射
* `std::mutex mutex_` → 线程安全保护
* `uint64_t next_id_` → 下一个分配的 ID

---


## TODO
测试在高通、英伟达平台上是否适配，habmm pmem

## SharedMemoryManager
摄像头、audio这样的数据都需要用共享内存的形式来管理，就涉及到每次刚进入数据的时候的共享内存分配，以及监测共享内存的计数，在引用计数降为0之后负责这一块共享内存的回收和销毁。

## SharedMemoryHandle
负责共享内存的具体数据结构。内部包含内存指针、大小和原子引用计数，支持移动语义和空状态检测。关键特性包括：自动引用计数，拷贝构造时计数+1，析构时计数-1；线程安全操作，所有方法都通过原子变量保证一致性；



### 子杰伪代码与接口

## SharedMemoryManager
| 描述               | 接口                                                                                                       |
|------------------|----------------------------------------------------------------------------------------------------------|
| 构造函数            | `SharedMemoryManager();`                                                                                  |
| 析构函数            | `~SharedMemoryManager();`                                                                                 |
| 分配共享内存          | `SharedMemoryHandle allocate(size_t size, ResourceType type);`                                            |
| 释放共享内存          | `bool deallocate(const SharedMemoryHandle& handle);`                                                      |
| 获取引用计数          | `int get_reference_count(const SharedMemoryHandle& handle);`                                              |
| 增加引用计数（拷贝句柄时） | `bool increase_reference_count(const SharedMemoryHandle& handle);`                                        |
| 创建现有内存的新句柄    | `SharedMemoryHandle create_handle_reference(const SharedMemoryHandle& original);`                         |


## SharedMemoryHandle
| 描述                 | 接口                                                          |
|--------------------|-------------------------------------------------------------|
| 默认构造函数            | `SharedMemoryHandle();`                                      |
| 构造函数：创建有效句柄     | `SharedMemoryHandle(uint64_t id, void* ptr, size_t size);` |
| 移动构造函数            | `SharedMemoryHandle(SharedMemoryHandle&& other) noexcept;`    |
| 移动赋值运算符           | `SharedMemoryHandle& operator=(SharedMemoryHandle&& other) noexcept;` |
| 禁用拷贝构造函数          | `SharedMemoryHandle(const SharedMemoryHandle&) = delete;`     |
| 禁用拷贝赋值运算符        | `SharedMemoryHandle& operator=(const SharedMemoryHandle&) = delete;` |
| 检查句柄有效性           | `bool is_valid() const;`                                     |
| 获取内存指针            | `void* get_ptr() const;`                                     |
| 获取内存大小            | `size_t get_size() const;`                                   |
| 获取句柄 ID            | `uint64_t get_id() const;`                                   |

``` c++
// 共享内存引用信息
struct RefCountedMemory {
    void* ptr;
    size_t size;
    ResourceType type;
    std::atomic<int> ref_count;
    uint64_t handle_id;  // 唯一标识符
    
    RefCountedMemory(void* p, size_t s, ResourceType t, uint64_t id) 
        : ptr(p), size(s), type(t), ref_count(1), handle_id(id) {}
};

// 共享内存句柄
class SharedMemoryHandle {
private:
    uint64_t handle_id;
    void* memory_ptr;
    size_t memory_size;
    bool valid;

public:
    SharedMemoryHandle() : handle_id(0), memory_ptr(nullptr), memory_size(0), valid(false) {}
    
    SharedMemoryHandle(uint64_t id, void* ptr, size_t size) 
        : handle_id(id), memory_ptr(ptr), memory_size(size), valid(true) {}
    
    // 移动语义
    SharedMemoryHandle(SharedMemoryHandle&& other) noexcept 
        : handle_id(other.handle_id), memory_ptr(other.memory_ptr), 
          memory_size(other.memory_size), valid(other.valid) {
        other.valid = false;
    }
    
    SharedMemoryHandle& operator=(SharedMemoryHandle&& other) noexcept {
        if (this != &other) {
            handle_id = other.handle_id;
            memory_ptr = other.memory_ptr;
            memory_size = other.memory_size;
            valid = other.valid;
            other.valid = false;
        }
        return *this;
    }
    
    // 禁用拷贝
    SharedMemoryHandle(const SharedMemoryHandle&) = delete;
    SharedMemoryHandle& operator=(const SharedMemoryHandle&) = delete;
    
    bool is_valid() const { return valid; }
    void* get_ptr() const { return memory_ptr; }
    size_t get_size() const { return memory_size; }
    uint64_t get_id() const { return handle_id; }
};

// 简化的共享内存管理器
class SharedMemoryManager {
private:
    std::unordered_map<uint64_t, std::unique_ptr<RefCountedMemory>> memory_blocks;
    mutable std::shared_mutex manager_mutex;
    std::atomic<uint64_t> next_handle_id{1};
    
public:
    SharedMemoryManager() = default;
    ~SharedMemoryManager() = default;

    // 分配共享内存
    SharedMemoryHandle allocate(size_t size, ResourceType type) {
        std::unique_lock<std::shared_mutex> lock(manager_mutex);
        
        // 1. 分配实际的共享内存
        void* ptr = allocate_actual_shared_memory(size);
        if (ptr == nullptr) {
            return SharedMemoryHandle();  // 分配失败，返回无效句柄
        }
        
        // 2. 生成唯一ID
        uint64_t handle_id = next_handle_id++;
        
        // 3. 创建引用计数记录
        auto memory_ref = std::make_unique<RefCountedMemory>(ptr, size, type, handle_id);
        memory_blocks[handle_id] = std::move(memory_ref);
        
        // 4. 返回句柄
        return SharedMemoryHandle(handle_id, ptr, size);
    }
    
    // 释放共享内存
    bool deallocate(const SharedMemoryHandle& handle) {
        if (!handle.is_valid()) {
            return false;
        }
        
        std::unique_lock<std::shared_mutex> lock(manager_mutex);
        
        auto it = memory_blocks.find(handle.get_id());
        if (it == memory_blocks.end()) {
            return false;  // 句柄不存在
        }
        
        auto& memory_ref = it->second;
        
        // 减少引用计数
        int new_count = --memory_ref->ref_count;
        
        // 如果引用计数为0，释放内存
        if (new_count <= 0) {
            free_actual_shared_memory(memory_ref->ptr, memory_ref->size);
            memory_blocks.erase(it);
        }
        
        return true;
    }
    
    // 获取引用计数
    int get_reference_count(const SharedMemoryHandle& handle) {
        if (!handle.is_valid()) {
            return 0;
        }
        
        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        
        auto it = memory_blocks.find(handle.get_id());
        if (it != memory_blocks.end()) {
            return it->second->ref_count.load();
        }
        
        return 0;
    }
    
    // 增加引用计数（用于句柄拷贝时）
    bool increase_reference_count(const SharedMemoryHandle& handle) {
        if (!handle.is_valid()) {
            return false;
        }
        
        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        
        auto it = memory_blocks.find(handle.get_id());
        if (it != memory_blocks.end()) {
            ++it->second->ref_count;
            return true;
        }
        
        return false;
    }
    
    // 创建现有内存的新句柄（增加引用计数）
    SharedMemoryHandle create_handle_reference(const SharedMemoryHandle& original) {
        if (!original.is_valid()) {
            return SharedMemoryHandle();
        }
        
        if (increase_reference_count(original)) {
            return SharedMemoryHandle(original.get_id(), original.get_ptr(), original.get_size());
        }
        
        return SharedMemoryHandle();
    }

private:
    // 实际分配共享内存的平台相关代码
    void* allocate_actual_shared_memory(size_t size) {
        // 伪代码：使用mmap或其他共享内存API
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        return (ptr == MAP_FAILED) ? nullptr : ptr;
        
        // 简化实现，实际使用时替换为真实的共享内存分配
        return malloc(size);
    }
    
    // 实际释放共享内存的平台相关代码
    void free_actual_shared_memory(void* ptr, size_t size) {
        // 伪代码：使用munmap或其他共享内存API
        munmap(ptr, size);
        
        // 简化实现，实际使用时替换为真实的共享内存释放
        free(ptr);
    }
};

