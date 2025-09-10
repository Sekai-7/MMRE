#include <sys/mman.h>

#include <cstring>

#include <iostream>
#include <map>
#include <deque>
#include <vector>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <algorithm>


#include <cstdlib>
#include <cstring>

using std::cerr;
using std::vector;
using std::map;
using std::deque;
using std::atomic;
using std::atomic_flag;
using std::shared_ptr;
using std::make_shared;

// 枚举表示数据类型
enum class DataType {
    CAMERA,
    AUDIO,
    // ... 其他数据类型
};

// 统一数据包结构
// 这个ds需要拷贝构造吗？？？
// struct UnifiedDataPacket {
//     long long timestamp;
//     DataType type;
//     void* data_ptr; // 指向共享内存的指针
//     size_t data_size;

//     UnifiedDataPacket() : timestamp(0), type(DataType::CAMERA), data_ptr(nullptr), data_size(0) {} // 添加默认构造函数
//     UnifiedDataPacket(long long ts, DataType t, void* ptr, size_t size) : timestamp(ts), type(t), data_ptr(ptr), data_size(size) {} // 添加构造函数

//     // 拷贝构造函数 (深拷贝共享内存)
//     UnifiedDataPacket(const UnifiedDataPacket& other) : timestamp(other.timestamp), type(other.type), data_size(other.data_size) {
//         if (other.data_ptr && other.data_size > 0) {
//             data_ptr = mmap(nullptr, other.data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//             if (data_ptr == MAP_FAILED) {
//                 cerr << "mmap failed in copy constructor";
//             }
//             memcpy(data_ptr, other.data_ptr, other.data_size);
//         } else {
//             data_ptr = nullptr;
//         }
//     }

//     // 赋值运算符重载 (深拷贝共享内存)
//     UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {
//         if (&other == this) {
//             return *this;
//         }
//         if (data_ptr != nullptr) {
//             munmap(data_ptr, data_size);
//         }
//         if (other.data_ptr && other.data_size > 0) {
//             data_ptr = mmap(nullptr, other.data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//             if (data_ptr == MAP_FAILED) {
//                 cerr << "mmap failed in copy constructor";
//             }
//             memcpy(data_ptr, other.data_ptr, other.data_size);
//         } else {
//             data_ptr = nullptr;
//         }
//         return *this;
//     }

//     // 析构函数 (释放共享内存)
//     ~UnifiedDataPacket() {
//         if (data_ptr) {
//             munmap(data_ptr, data_size);
//         }
//     }
// };
struct UnifiedDataPacket {
    long long timestamp;
    DataType type;
    void* data_ptr; // 指向数据
    size_t data_size;

    UnifiedDataPacket() : timestamp(0), type(DataType::CAMERA), data_ptr(nullptr), data_size(0) {}

    UnifiedDataPacket(long long ts, DataType t, const void* ptr, size_t size)
        : timestamp(ts), type(t), data_size(size) {
        if (ptr && size > 0) {
            data_ptr = malloc(size);
            if (!data_ptr) throw std::bad_alloc();
            memcpy(data_ptr, ptr, size);
        } else {
            data_ptr = nullptr;
        }
    }

    // 拷贝构造 (深拷贝)
    UnifiedDataPacket(const UnifiedDataPacket& other)
        : timestamp(other.timestamp), type(other.type), data_size(other.data_size) {
        if (other.data_ptr && other.data_size > 0) {
            data_ptr = malloc(other.data_size);
            if (!data_ptr) throw std::bad_alloc();
            memcpy(data_ptr, other.data_ptr, other.data_size);
        } else {
            data_ptr = nullptr;
        }
    }

    // 赋值运算符 (深拷贝)
    UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {
        if (&other == this) return *this;
        if (data_ptr) free(data_ptr);
        timestamp = other.timestamp;
        type = other.type;
        data_size = other.data_size;
        if (other.data_ptr && other.data_size > 0) {
            data_ptr = malloc(other.data_size);
            if (!data_ptr) throw std::bad_alloc();
            memcpy(data_ptr, other.data_ptr, other.data_size);
        } else {
            data_ptr = nullptr;
        }
        return *this;
    }

    ~UnifiedDataPacket() {
        if (data_ptr) free(data_ptr);
    }
};

// find出来的数据是否会被修改呢？
// 还是只是需要拿到数据？
class TimelineCacheBaseLine {
private:
    map<long long, shared_ptr<UnifiedDataPacket>> data_map;
    deque<long long> data_deque;

public:
    void insert(long long timestamp, DataType type, void* data_ptr, size_t data_size);

    shared_ptr<UnifiedDataPacket> find(long long timestamp);

    vector<shared_ptr<UnifiedDataPacket>> query_range(long long start_ts, long long end_ts);

    shared_ptr<UnifiedDataPacket> evict_oldest();
    
    void insertCameraData(long long timestamp, const char* image_data, size_t image_size);

    void insertAudioData(long long timestamp, const char* audio_data, size_t audio_size);

    ~TimelineCacheBaseLine() {}
};

// struct CacheNode {
//     // 键与值
//     long long timestamp;                // 键: 时间戳, 用于树的排序
//     UnifiedDataPacket packet;           // 值: 统一数据包

//     // --- 树相关指针 ---
//     CacheNode* parent;                  // 指向父节点
//     CacheNode* left;                    // 指向左子节点
//     CacheNode* right;                   // 指向右子节点
//     int height;                         // 节点高度 (用于AVL树的平衡)

//     // --- 链表相关指针 ---
//     CacheNode* prev_by_time;            // 指向前一个节点 (按时间戳排序)
//     CacheNode* next_by_time;            // 指向后一个节点 (按时间戳排序)

//     // --- copy on write相关字段 ---
//     std::atomic<int> ref_count{1};      // 引用计数，用于COW
//     bool is_shared{false};              // 标记节点是否被多个版本共享
// };

// class TimelineCache {
// private:
//     CacheNode* root;                    // 树的根节点

//     CacheNode* list_head;               // 链表的头节点 (时间戳最小的节点)
//     CacheNode* list_tail;               // 链表的尾节点 (时间戳最大的节点)

//     long long current_size;             // 缓存中元素的数量
//     long long current_memory_usage;     // 缓存占用的总内存大小

//     CacheNodePool& node_pool;           // 引用外部内存池

//     // 用于看要不要访问持久化数据
//     long long min_timestamp;
//     long long max_timestamp;
    
//     // copy-on-write相关字段
//     atomic<CacheNode*> root_for_read{nullptr};   // 读操作使用的根节点
//     CacheNode* root_for_write{nullptr};              // 写操作使用的根节点
//     atomic<int> active_readers{0};               // 活跃读者计数
//     atomic_flag write_lock = ATOMIC_FLAG_INIT;   // 写操作锁

//     // --- 私有辅助函数 ---
//     // (例如: 旋转, 平衡, 递归插入/删除等)
//     CacheNode* rotate_left(CacheNode* node);
//     CacheNode* rotate_right(CacheNode* node);
//     CacheNode* rebalance(CacheNode* node);
//     CacheNode* insert(CacheNode* node, long long timestamp, UnifiedDataPacket packet);
//     CacheNode* remove(CacheNode* node, long long timestamp);
//     void query_range(CacheNode* node, long long start_ts, long long end_ts, std::vector<UnifiedDataPacket>& results);
//     int get_height(CacheNode* node);
//     int get_balance_factor(CacheNode* node);

//     // COW辅助函数
//     CacheNode* clone_node_if_shared(CacheNode* node);
//     void increment_ref_count(CacheNode* node);
//     void decrement_ref_count(CacheNode* node);

// public:
//     // --- 公共接口 ---
//         // 构造函数：接受外部内存池的引用
//     explicit TimelineCache(CacheNodePool& pool) : 
//         root(nullptr), list_head(nullptr), list_tail(nullptr),
//         current_size(0), current_memory_usage(0), 
//         min_timestamp(LLONG_MAX), max_timestamp(LLONG_MIN),
//         node_pool(pool) {}
//     ~TimelineCache(); // 需要实现以释放所有节点

//     void insert(long long timestamp, UnifiedDataPacket packet);
//     bool remove(long long timestamp);
//     UnifiedDataPacket find(long long timestamp);
//     std::vector<UnifiedDataPacket> query_range(long long start_ts, long long end_ts);

//     // 淘汰策略接口
//     UnifiedDataPacket evict_oldest(); // O(1)定位 + O(log N)删除
//     UnifiedDataPacket evict_newest(); // O(1)定位 + O(log N)删除

//     // 用于看要不要访问持久化数据
//     long long get_min_timestamp() const { return min_timestamp; }
//     long long get_max_timestamp() const { return max_timestamp; }

//     long long size();
//     long long memory_usage();
// };




// ------------------ 内存池占位 ------------------
//struct CacheNodePool {};
// 在 TimelineCacheBenchmark.h 或者你的公共头文件中


// ------------------ TimelineCache ------------------
class TimelineCache {
private:
    CacheNode* root;
    CacheNode* list_head;
    CacheNode* list_tail;

    long long current_size;
    long long current_memory_usage;

    CacheNodePool& node_pool;

    long long min_timestamp;
    long long max_timestamp;

    // --- AVL 辅助 ---
    static int get_height(CacheNode* n);
    static int get_balance_factor(CacheNode* n);
    static void update_height(CacheNode* n);

    CacheNode* rotate_left(CacheNode* x);
    CacheNode* rotate_right(CacheNode* y);
    CacheNode* rebalance(CacheNode* n);

    CacheNode* insert(CacheNode* node, long long ts, const UnifiedDataPacket& packet, CacheNode*& inserted);
    CacheNode* remove_pureAVL(CacheNode* node, long long ts, CacheNode*& deleted_node);

    void query_range(CacheNode* node, long long l, long long r, std::vector<UnifiedDataPacket>& out) const;

    void destroy(CacheNode* node);
    CacheNode* find_node(long long ts) const;

    CacheNode* find_predecessor(long long ts) const;
    CacheNode* find_successor(long long ts) const;

    void link_into_list(CacheNode* node);
    void unlink_from_list(CacheNode* node);
    void relink_into_list(CacheNode* node);

    void refresh_min_max_after_change();

public:
    explicit TimelineCache(CacheNodePool& pool);
    ~TimelineCache();

    void insert(long long timestamp, UnifiedDataPacket packet);
    bool remove(long long timestamp);
    UnifiedDataPacket find(long long timestamp);
    std::vector<UnifiedDataPacket> query_range(long long start_ts, long long end_ts);

    UnifiedDataPacket evict_oldest();
    UnifiedDataPacket evict_newest();

    long long get_min_timestamp() const;
    long long get_max_timestamp() const;

    long long size();
    long long memory_usage();
};