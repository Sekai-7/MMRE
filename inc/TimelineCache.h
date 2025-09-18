#include "SharedMemoryManager.h"
#include <sys/mman.h>

#include <iostream>
#include <map>
#include <deque>
#include <vector>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <variant>


#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstring>
#include <ctime>


struct CacheNode;
struct CacheNodePool;

using Timestamp = std::time_t;
using Handle = std::variant<void*, SharedMemoryHandle>;

// 枚举表示数据类型
enum class ResourceType {
    CAMERA,
    AUDIO,
    GPS,
    IMU,
    VEHICLE_SIGNAL
    // ... 其他数据类型
};

// 统一数据包结构
struct UnifiedDataPacket {
    Timestamp timestamp;
    ResourceType type;
    // void* data_ptr; // 指向共享内存的指针
    Handle data_ptr;
    size_t data_size;

    void* get_ptr() {
        struct Visitor {
            void* operator() (void* ptr) {
                return ptr; 
            }
            void* operator() (SharedMemoryHandle handle) {
                if (handle.is_valid() == false || handle.get_size() == 0)
                    return nullptr;
                return handle.get_ptr();
            }
        };
        return std::visit(Visitor{}, data_ptr);
    }

    const void* get_ptr() const {
        struct Visitor {
            void* operator() (void* ptr) {
                return ptr; 
            }
            void* operator() (SharedMemoryHandle handle) {
                if (handle.is_valid() == false || handle.get_size() == 0)
                    return nullptr;
                return handle.get_ptr();
            }
        };
        return std::visit(Visitor{}, data_ptr);
    }

    UnifiedDataPacket() : timestamp(0), type(ResourceType::CAMERA), data_ptr(nullptr), data_size(0) {} // 添加默认构造函数
    UnifiedDataPacket(Timestamp ts, ResourceType t, void* ptr, size_t size) : timestamp(ts), type(t), data_ptr(ptr), data_size(size) {} // 添加构造函数

    UnifiedDataPacket(const UnifiedDataPacket& other) = delete;
    UnifiedDataPacket& operator=(const UnifiedDataPacket& other) = delete;
    // UnifiedDataPacket(const UnifiedDataPacket& other) {
    //     this->timestamp = other.timestamp;
    //     this->type = other.type;
    //     this->data_size = other.data_size;
    //     if (std::get_if<SharedMemoryHandle>(&other.data_ptr)) {
            
    //     }

    // }
    // UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {
    //     this->timestamp = other.timestamp;
    //     this->type = other.type;
    //     this->data_size = other.data_size;
    //     this->data_ptr = std::move(other.data_ptr);

    // }

    UnifiedDataPacket(UnifiedDataPacket&& other) {
        this->timestamp = other.timestamp;
        this->type = other.type;
        this->data_size = other.data_size;
        this->data_ptr = std::move(other.data_ptr);
        return;
    }

    UnifiedDataPacket& operator=(UnifiedDataPacket&& other) {
        if (&other == this)
            return *this;
        this->timestamp = other.timestamp;
        this->type = other.type;
        this->data_size = other.data_size;
        this->data_ptr = std::move(other.data_ptr);
        return *this;
    }

    // 拷贝构造函数 (深拷贝共享内存)
    // UnifiedDataPacket(const UnifiedDataPacket& other) : timestamp(other.timestamp), type(other.type), data_size(other.data_size) {}

    // 赋值运算符重载 (深拷贝共享内存)
    // UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {}

    // 析构函数 (释放共享内存)
    ~UnifiedDataPacket() {}
};

// find出来的数据是否会被修改呢？
// 还是只是需要拿到数据？
// class TimelineCacheBaseLine {
// private:
//     std::map<Timestamp, std::shared_ptr<UnifiedDataPacket>> data_map;
//     std::deque<Timestamp> data_deque;

//     std::mutex mtx;

// public:
//     void insert(Timestamp, ResourceType, void*, size_t);

//     std::shared_ptr<UnifiedDataPacket> find(Timestamp);

//     std::vector<std::shared_ptr<UnifiedDataPacket>> query_range(Timestamp, Timestamp);

//     std::shared_ptr<UnifiedDataPacket> evict_oldest();

//     void clear();
    
//     ~TimelineCacheBaseLine() {}
// };

// ------------------ TimelineCache ------------------
class TimelineCache {
private:
    std::mutex mtx;

    CacheNode* root;
    CacheNode* list_head;
    CacheNode* list_tail;

    Timestamp current_size;
    Timestamp current_memory_usage;

    CacheNodePool& node_pool;

    Timestamp min_timestamp;
    Timestamp max_timestamp;

    // --- AVL 辅助 ---
    static int get_height(CacheNode* n);
    static int get_balance_factor(CacheNode* n);
    static void update_height(CacheNode* n);

    CacheNode* rotate_left(CacheNode* x);
    CacheNode* rotate_right(CacheNode* y);
    CacheNode* rebalance(CacheNode* n);

    CacheNode* insert(CacheNode* node, Timestamp ts, UnifiedDataPacket&& packet, CacheNode*& inserted);
    CacheNode* remove_pureAVL(CacheNode* node, Timestamp ts, CacheNode*& deleted_node);

    void query_range(CacheNode* node, Timestamp, Timestamp, std::vector<UnifiedDataPacket*>& out) const;

    void destroy(CacheNode* node);
    CacheNode* find_node(Timestamp ts) const;

    CacheNode* find_predecessor(Timestamp ts) const;
    CacheNode* find_successor(Timestamp ts) const;

    void link_into_list(CacheNode* node);
    void unlink_from_list(CacheNode* node);
    void relink_into_list(CacheNode* node);

    void refresh_min_max_after_change();

public:
    explicit TimelineCache(CacheNodePool& pool);
    ~TimelineCache();

    void clear();

    void insert(UnifiedDataPacket&& packet);
    bool remove(Timestamp timestamp);
    UnifiedDataPacket* query(Timestamp timestamp);
    std::vector<UnifiedDataPacket*> query_by_range(Timestamp start_ts, Timestamp end_ts);

    UnifiedDataPacket evict_oldest();
    UnifiedDataPacket evict_newest();

    Timestamp get_min_timestamp() const;
    Timestamp get_max_timestamp() const;

    long long size();
    long long memory_usage();
};