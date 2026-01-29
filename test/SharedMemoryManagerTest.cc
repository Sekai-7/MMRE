#include <gtest/gtest.h>
#include "SharedMemoryManager.h"
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <cstdlib>

class SharedMemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 模拟 Docker 环境环境变量，防止构造函数直接退出
        // 注意：这只影响当前进程，且必须在 SharedMemoryManager 实例化前设置
        // setenv("MMRE_CONTAINER", "1", 1);
        
        manager = new SharedMemoryManager();
    }

    void TearDown() override {
        delete manager;
    }

    SharedMemoryManager* manager;
};

// 测试基本的分配功能
TEST_F(SharedMemoryManagerTest, AllocateBasic) {
    size_t size = 1024; // 1KB
    SharedMemoryHandle handle = manager->allocate(size);

    EXPECT_TRUE(handle.isValid());
    EXPECT_NE(handle.getPtr(), nullptr);
    EXPECT_EQ(handle.getSize(), size); // Handle 应该返回用户请求的真实大小
    EXPECT_GT(handle.getId(), 0);

    // 测试写入和读取
    char* data = static_cast<char*>(handle.getPtr());
    const char* testStr = "Hello, Shared Memory!";
    std::strcpy(data, testStr);
    EXPECT_STREQ(data, testStr);
}

// 测试不同大小的内存对齐分配
TEST_F(SharedMemoryManagerTest, AllocateAlignedSizes) {
    // 测试 4KB
    SharedMemoryHandle h1 = manager->allocate(4096);
    EXPECT_TRUE(h1.isValid());
    EXPECT_EQ(h1.getSize(), 4096);

    // 测试 512KB
    SharedMemoryHandle h2 = manager->allocate(512 * 1024);
    EXPECT_TRUE(h2.isValid());
    EXPECT_EQ(h2.getSize(), 512 * 1024);
    
    // 测试不规则大小，应该向上对齐（内部实现），但 Handle 应该记录真实请求大小
    SharedMemoryHandle h3 = manager->allocate(4097);
    EXPECT_TRUE(h3.isValid());
    // Handle 存储的是用户请求的真实大小，而不是内部对齐后的大小
    EXPECT_EQ(h3.getSize(), 4097);
}

// 测试引用计数和拷贝构造
TEST_F(SharedMemoryManagerTest, CopyConstructorAndRefCount) {
    SharedMemoryHandle h1 = manager->allocate(1024);
    ASSERT_TRUE(h1.isValid());
    char* ptr1 = static_cast<char*>(h1.getPtr());
    uint64_t id1 = h1.getId();

    std::strcpy(ptr1, "Persist");

    {
        // 拷贝构造，引用计数+1
        SharedMemoryHandle h2 = h1;
        EXPECT_EQ(h2.getId(), id1);
        EXPECT_EQ(h2.getPtr(), ptr1);
        EXPECT_STREQ(static_cast<char*>(h2.getPtr()), "Persist");
    } // h2 析构，引用计数-1，但内存不应释放

    // h1 仍然有效
    EXPECT_STREQ(static_cast<char*>(h1.getPtr()), "Persist");
    
    // 再次通过 ID 验证（虽然不能直接访问 private 成员，但可以通过再次拷贝验证）
    SharedMemoryHandle h3 = h1;
    EXPECT_EQ(h3.getPtr(), ptr1);
}

// 测试赋值操作符
TEST_F(SharedMemoryManagerTest, AssignmentOperator) {
    SharedMemoryHandle h1 = manager->allocate(1024);
    SharedMemoryHandle h2(manager); // 无效句柄

    EXPECT_TRUE(h1.isValid());
    EXPECT_FALSE(h2.isValid());

    h2 = h1; // 赋值
    EXPECT_TRUE(h2.isValid());
    EXPECT_EQ(h2.getId(), h1.getId());
    EXPECT_EQ(h2.getPtr(), h1.getPtr());
}

// 测试移动语义
TEST_F(SharedMemoryManagerTest, MoveSemantics) {
    SharedMemoryHandle h1 = manager->allocate(1024);
    void* ptr1 = h1.getPtr();
    uint64_t id1 = h1.getId();

    SharedMemoryHandle h2 = std::move(h1);

    // h2 应该接管资源
    EXPECT_TRUE(h2.isValid());
    EXPECT_EQ(h2.getPtr(), ptr1);
    EXPECT_EQ(h2.getId(), id1);

    // h1 应该变为空/无效
    EXPECT_FALSE(h1.isValid());
    EXPECT_EQ(h1.getPtr(), nullptr);
}

// 测试内存复用 (Free List)
TEST_F(SharedMemoryManagerTest, MemoryReuse) {
    void* ptr1;
    {
        SharedMemoryHandle h1 = manager->allocate(4096);
        ptr1 = h1.getPtr();
        // h1 析构，归还到 freeBlocks[0] (4KB slot)
    }

    SharedMemoryHandle h2 = manager->allocate(4096);
    // 如果复用逻辑正确，h2 应该拿到相同的地址 (这是由当前简单的 Stack/LIFO free list 实现决定的)
    // 注意：如果这是并发环境可能不一定，但单线程测试通常如此
    EXPECT_EQ(h2.getPtr(), ptr1);
}

// 测试超出预设大小的分配 (当前实现可能会失败或返回特定值)
TEST_F(SharedMemoryManagerTest, LargeAllocation) {
    // 12MB 是当前代码的一个分界线
    // 尝试分配 13MB
    size_t size = 13 * 1024 * 1024;
    SharedMemoryHandle h = manager->allocate(size);

    // 根据代码逻辑，sizeToIdx 返回 -1，allocate 返回无效句柄
    // 这里测试是否符合预期的"失败"行为
    EXPECT_FALSE(h.isValid());
    EXPECT_EQ(h.getPtr(), nullptr);
}

// 测试多个分配
TEST_F(SharedMemoryManagerTest, MultipleAllocations) {
    std::vector<SharedMemoryHandle> handles;
    for(int i=0; i<10; ++i) {
        handles.push_back(manager->allocate(4096));
        EXPECT_TRUE(handles.back().isValid());
        // 简单写入，确保内存块不同（或者如果不重叠）
        int* intPtr = static_cast<int*>(handles.back().getPtr());
        *intPtr = i;
    }

    for(int i=0; i<10; ++i) {
        int* intPtr = static_cast<int*>(handles[i].getPtr());
        EXPECT_EQ(*intPtr, i);
    }
}

// [BUG REPRODUCTION] 测试 RefCount 初始化 Bug：拷贝对象析构导致内存过早释放
TEST_F(SharedMemoryManagerTest, RefCountInitializationBug) {
    // 1. 分配一个 4KB 块
    SharedMemoryHandle h1 = manager->allocate(4096);
    ASSERT_TRUE(h1.isValid());
    void* originalPtr = h1.getPtr();

    // 2. 局部作用域内拷贝 h1 -> h2
    {
        SharedMemoryHandle h2 = h1; 
        // 此时，如果 bug 存在，底层 MemoryBlock 的 refCount 可能是 1 (0 + 1)，而不是预期的 2 (1 + 1)
        // 或者是：如果 allocate 没有初始化 refCount 为 1，则初始为 0。拷贝后变成 1。
    } 
    // h2 析构。 refCount --。
    // 如果 refCount 变成了 0，Block 会被 deallocate 回收到 freeList 中。

    // 3. 此时 h1 仍然存活，它应该仍然拥有这个 Block。
    // 验证方法：请求一个新的分配。如果是 Stack/LIFO 策略，且 Block 被错误释放了，新分配会拿到同一个地址。
    
    SharedMemoryHandle h3 = manager->allocate(4096);
    void* newPtr = h3.getPtr();

    // 如果 bug 存在，newPtr 将等于 originalPtr，意味着 h3 复用了 h1 正在使用的内存！
    // 如果 bug 修复，Block 被 h1 占用，h3 必须拿到一个新的块。
    EXPECT_NE(newPtr, originalPtr) 
        << "CRITICAL FAILURE: Memory Block was prematurely freed! "
        << "h1 is still alive but h3 received the same memory address. "
        << "This indicates the reference count dropped to zero when the copy (h2) was destroyed.";
}