# 缓存数据结构 (Cache Structure) 详尽设计
![alt text](image-2.png)
## 说明
首先这里用到的数据结构肯定是O(log N) 插入/删除级别的，但不同的数据结构会有常数上的不同，这个设计也是尽可能减少各个操作在常数级别的性能开销。

最简单基础的、直接完全用c++的std实现的一个方案可以作为我们的baseline。

先提供一个这个baseline方案的伪代码，也可以有助于抛开数据结构内部的复杂操作，理解该过程本身的逻辑。

``` c++
#include <iostream>
#include <map>
#include <deque>
#include <vector>
#include <sys/mman.h> // for shared memory

// 枚举表示数据类型
enum DataType {
    CAMERA,
    AUDIO,
    // ... 其他数据类型
};

// 统一数据包结构
struct UnifiedDataPacket {
    long long timestamp;
    DataType type;
    void* data_ptr; // 指向共享内存的指针
    size_t data_size;

    UnifiedDataPacket() : timestamp(0), type(CAMERA), data_ptr(nullptr), data_size(0) {} // 添加默认构造函数
    UnifiedDataPacket(long long ts, DataType t, void* ptr, size_t size) : timestamp(ts), type(t), data_ptr(ptr), data_size(size) {} // 添加构造函数

    // 拷贝构造函数 (深拷贝共享内存)
    UnifiedDataPacket(const UnifiedDataPacket& other) : timestamp(other.timestamp), type(other.type), data_size(other.data_size) {
        if (other.data_ptr && other.data_size > 0) {
            data_ptr = mmap(NULL, other.data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (data_ptr == MAP_FAILED) {
                perror("mmap failed in copy constructor");
                // ... error handling
            }
            memcpy(data_ptr, other.data_ptr, other.data_size);
        } else {
            data_ptr = nullptr;
        }
    }

    // 赋值运算符重载 (深拷贝共享内存)
    UnifiedDataPacket& operator=(const UnifiedDataPacket& other) {
        if (this != &other) { // self-assignment check
            if (data_ptr) {
                munmap(data_ptr, data_size); // 释放旧的共享内存
            }
            timestamp = other.timestamp;
            type = other.type;
            data_size = other.data_size;
            if (other.data_ptr && other.data_size > 0) {
                data_ptr = mmap(NULL, other.data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (data_ptr == MAP_FAILED) {
                    perror("mmap failed in assignment operator");
                    // ... error handling
                }
                memcpy(data_ptr, other.data_ptr, other.data_size);
            } else {
                data_ptr = nullptr;
            }
        }
        return *this;
    }

    // 析构函数 (释放共享内存)
    ~UnifiedDataPacket() {
        if (data_ptr) {
            munmap(data_ptr, data_size);
        }
    }
};


class TimelineCacheBaseLine {
private:
    std::map<long long, UnifiedDataPacket> data_map;
    std::deque<UnifiedDataPacket*> data_deque;

public:
    void insert(long long timestamp, DataType type, void* data_ptr, size_t data_size) {
        UnifiedDataPacket packet{timestamp, type, data_ptr, data_size};
        data_map[timestamp] = packet;
        data_deque.push_back(&data_map[timestamp]);
    }

    UnifiedDataPacket* find(long long timestamp) {
        // 使用 map 进行查找 (O(log N))
        auto it = data_map.find(timestamp);
        if (it != data_map.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::vector<UnifiedDataPacket> query_range(long long start_ts, long long end_ts) {
        // 利用 map 的有序性进行范围查询 (O(log N + K))
        std::vector<UnifiedDataPacket> results;
        auto it_start = data_map.lower_bound(start_ts);
        auto it_end = data_map.upper_bound(end_ts);
        for (auto it = it_start; it != it_end; ++it) {
            results.push_back(it->second);
        }
        return results;
    }

    UnifiedDataPacket evict_oldest() {
        if (data_deque.empty()) {
            throw std::runtime_error("Cache is empty");
        }
        UnifiedDataPacket* oldest_packet = data_deque.front();
        data_deque.pop_front();
        UnifiedDataPacket packet_to_return = *oldest_packet; 
        // 2. 从 map 中删除对应的数据 (O(log N))。这里需要注意的是，由于deque中存储的是指针，因此删除map中的元素后，deque中的指针会失效，所以需要先拷贝数据再删除
        data_map.erase(oldest_packet->timestamp);
        return packet_to_return;
    }
    void insertCameraData(long long timestamp, const char* image_data, size_t image_size) {
        void* shm_ptr = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (shm_ptr == MAP_FAILED) {
            perror("mmap failed");
            // ... error handling
        }
        memcpy(shm_ptr, image_data, image_size);
        insert(timestamp, CAMERA, shm_ptr, image_size);
    }

    void insertAudioData(long long timestamp, const char* audio_data, size_t audio_size) {
        void* shm_ptr = mmap(NULL, audio_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (shm_ptr == MAP_FAILED) {
            perror("mmap failed");
            // ... error handling
        }
        memcpy(shm_ptr, audio_data, audio_size);
        insert(timestamp, AUDIO, shm_ptr, audio_size);
    }

    ~TimelineCache() {
        //  不需要手动释放共享内存，UnifiedDataPacket的析构函数会处理
    }
};

int main() {
    // ... (测试代码)
       TimelineCache cache;

    // 示例数据
    char camera_data[] = "Camera Image Data";
    char audio_data[] = "Audio Waveform Data";

    cache.insertCameraData(10, camera_data, sizeof(camera_data));
    cache.insertAudioData(20, audio_data, sizeof(audio_data));

    // 测试 find()
    UnifiedDataPacket* found_camera = cache.find(10);
    if (found_camera) {
        std::cout << "Found Camera Data at timestamp 10: " << static_cast<char*>(found_camera->data_ptr) << std::endl;
    }

    UnifiedDataPacket* found_audio = cache.find(20);
    if (found_audio) {
        std::cout << "Found Audio Data at timestamp 20: " << static_cast<char*>(found_audio->data_ptr) << std::endl;
    }

    // 测试 query_range()
    std::vector<UnifiedDataPacket> range_results = cache.query_range(5, 25);
    std::cout << "Data in range [5, 25]:\n";
    for (const auto& packet : range_results) {
        std::cout << "Timestamp: " << packet.timestamp << ", Data: " << static_cast<char*>(packet.data_ptr) << std::endl;
    }

    // 测试 evict_oldest()
    UnifiedDataPacket evicted = cache.evict_oldest();
    std::cout << "Evicted oldest data (timestamp " << evicted.timestamp << "): " << static_cast<char*>(evicted.data_ptr) << std::endl;

    // 再次尝试查找被删除的数据，应该找不到
    found_camera = cache.find(10);
    if (!found_camera) {
        std::cout << "Camera data at timestamp 10 not found (as expected)." << std::endl;
    }
}

```
这个baseline方案存在一些明显的缺陷：
- 数据冗余: data_deque 中存储的是指向 data_map 中 UnifiedDataPacket 的指针。这意味着每个数据包都被存储了两次（一次是实际数据，一次是指针），增加了内存开销。
- 同步开销: 在多线程环境下，对 data_map 和 data_deque 的并发访问需要额外的同步机制（例如互斥锁）来保证数据一致性，这会增加代码复杂性和性能开销。尤其在 evict_oldest 函数中，需要格外小心，避免 data_deque 中的指针失效。
- 效率问题: 虽然 std::deque 提供了 O(1) 的头尾访问，但每次插入和删除都需要同时操作 std::map 和 std::deque 两个容器，增加了操作的复杂性和总时间成本。

## 基于AVL树的升级结构
首先肯定的是，平衡树是最适合这个需求的数据结构，而手写的平衡树也会比直接使用封装好的STL有更好的灵活性和极致性能优化空间，所以这里我们采用手写平衡树的形式。

其次，平衡树有许多种类，我们要结合这个场景的性能需求与业务需求等等，选择最合适的基础类型。综合对比红黑树 Splay Treap B+ AVL等不同形式，我认为AVL是最合适的，主要有以下三点核心原因：
- 严格平衡保障查询性能：在 10ms 响应要求下，AVL 的 O(log n) 稳定低延迟是核心优势，尤其适合高频时间范围查询。

- 有序性场景优化：时间线本质是有序序列，AVL 在有序插入/删除和范围遍历上优于红黑树（抵消部分频繁旋转的性能 旋转更局部和可预测）。

- 好写：手写 AVL 的复杂度可控（旋转逻辑明确）。删除旧数据、找最小值等缓存管理操作更直接。

具体来说，为了满足“可查询所有资源 也可查询单一资源”的需求，我们为每种资源建立一颗单独的平衡树。

**锁的设计**

1. 场景特点：大数据规模，有查询和写入/删除操作，其中查询操作有较为严格的时间要求（要很快），而写入/删除操作的时间要求相对没有这么严格。但写入操作较为密集，overhead也不能不考虑。
2. 一个可能的设计：copy-on-write。平衡树写操作的copy-on-write空间复杂度是O(logn)的，可以接受。这样的话，读操作就是几乎无锁的，写操作也不会阻塞读操作。因为具体逻辑本身不会影响锁逻辑的实现，所以下面的伪代码示例中，仅仅在典型的写操作insert，和典型的读操作：基础递归式query_range两处示例了该锁设计方案的伪代码。
3. 写入每次会复制一条路径上的节点，这样旧的节点就可以回收再利用了，而因为我们是统一内存池管理的，这个回收利用也很简单，只用把这个node标志为已经回收，内存池内部的逻辑就能完成后续的回收工作了，简单来说就是把这个节点重新加入待分配的池就行。

### 1 核心数据结构定义
CacheNode (缓存节点)
这是我们数据结构的基本单元。每个节点既是树的一个节点，也是链表的一个环节。
``` c++
// 伪代码: CacheNode 结构定义
struct CacheNode {
    // 键与值
    long long timestamp;                // 键: 时间戳, 用于树的排序
    UnifiedDataPacket packet;           // 值: 统一数据包

    // --- 树相关指针 ---
    CacheNode* parent;                  // 指向父节点
    CacheNode* left;                    // 指向左子节点
    CacheNode* right;                   // 指向右子节点
    int height;                         // 节点高度 (用于AVL树的平衡)

    // --- 链表相关指针 ---
    CacheNode* prev_by_time;            // 指向前一个节点 (按时间戳排序)
    CacheNode* next_by_time;            // 指向后一个节点 (按时间戳排序)

    // --- copy on write相关字段 ---
    std::atomic<int> ref_count{1};      // 引用计数，用于COW
    bool is_shared{false};              // 标记节点是否被多个版本共享
};
```

TimelineCache (时间线缓存主类)
这个类封装了所有的操作逻辑。每种资源维护一个独立的内存池，因为不同的资源类型,其数据包的大小和访问频率可能会有很大差异，管理策略也不同，混在一起很容易出现内存碎片。
``` c++
class MultiResourceTimelineCache {
private:
    // 每种资源类型对应一个独立的 TimelineCache
    std::unordered_map<ResourceType, std::unique_ptr<TimelineCache>> resource_caches;

    // 共享内存管理器
    std::unique_ptr<SharedMemoryManager> shared_memory_manager;

    // 持久化管理
    std::unique_ptr<PersistenceEngine> persistence_engine;

    // 检查点管理器
    std::unique_ptr<MultiResourceCheckpointManager> checkpoint_manager;

    // 每个资源类型独立的读写锁
    mutable std::unordered_map<ResourceType, std::shared_mutex> resource_mutexes;
    std::shared_mutex resource_mutexes_lock; // 仅保护resource_mutexes本身

    // 缓存配置参数
    CacheConfiguration config;

public:
    explicit MultiResourceTimelineCache(const CacheConfiguration& cfg)
        : config(cfg)
        , shared_memory_manager(std::make_unique<SharedMemoryManager>())
        , checkpoint_manager(std::make_unique<MultiResourceCheckpointManager>())
        , persistence_engine(std::make_unique<PersistenceEngine>()) {
        //...正常初始化逻辑
        // ...
        // ...
        // 监测是否需要移除数据
        persistence_engine->set_monitored_cache(this);
        persistence_engine->start_monitoring();
    }

    // 插入数据
    bool insert_data(ResourceType type, Timestamp timestamp, const RawResourceData& raw_data) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        auto& cache = get_or_create_cache(type);
        bool success = cache->insert(timestamp, convert_to_unified_packet(type, timestamp, raw_data));
        return success;
    }

    // 查询数据
QueryResult query_by_range(ResourceType type, Timestamp start_ts, Timestamp end_ts) const {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        
        // 首先从内存缓存查询
        QueryResult cache_result;
        auto it = resource_caches.find(type);
        if (it != resource_caches.end()) {
            auto packets = it->second->query_range(start_ts, end_ts);
            cache_result = QueryResult::create_success(type, std::move(packets));
        } else {
            cache_result = QueryResult::create_empty(type);
        }
        
        // 检查是否需要从持久化存储查询
        // 如果缓存中没有找到完整数据，尝试从持久化存储查询
        if (cache_result.packets.empty() || 
            !is_range_completely_covered_by_cache(type, start_ts, end_ts)) {
            
            // 从持久化存储查询
            auto persisted_result = persistence_engine->query_persisted_data(type, start_ts, end_ts);
            
            // 合并缓存结果和持久化结果
            cache_result = merge_query_results(cache_result, persisted_result);
        }
        
        return cache_result;
    }

    MultiResourceQueryResult query_all_resources_by_range(Timestamp start_ts, Timestamp end_ts) const {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        MultiResourceQueryResult result;
        
        // 遍历所有资源类型
        for (const auto& resource_type : get_all_resource_types()) {
            auto query_result = query_by_range(resource_type, start_ts, end_ts);
            if (!query_result.packets.empty()) {
                result.add_resource_data(resource_type, std::move(query_result.packets));
            }
        }
        //...也添加上面类似的检查是否需要从持久化存储查询的逻辑
        
        return result;
    }

    // 立即持久化
    void trigger_immediate_persistence(ResourceType type) {
        persistence_engine->trigger_immediate_persistence(this, type);
    }


    // 检查点相关代码
    // 插入检查点
    bool insert_checkpoint(Timestamp timestamp) {
        return checkpoint_manager->insert_checkpoint(timestamp);
    }

    // 删除指定时间戳之前的检查点
    bool remove_checkpoints_before(Timestamp timestamp) {
        return checkpoint_manager->remove_checkpoints_before(timestamp) > 0;
    }

    // 查询指定时间戳之前的检查点
    std::vector<Timestamp> query_checkpoints_before(Timestamp timestamp) {
        return checkpoint_manager->query_checkpoints_before(timestamp);
    }

    // 查询指定时间范围内的检查点
    std::vector<Timestamp> query_checkpoints_in_range(Timestamp start, Timestamp end) {
        return checkpoint_manager->query_checkpoints_in_range(start, end);
    }

    // 查找最接近指定时间戳的检查点
    std::optional<Timestamp> find_nearest_checkpoint(Timestamp timestamp) {
        return checkpoint_manager->find_nearest_checkpoint(timestamp);
    }

    // 获取最新检查点
    std::optional<Timestamp> get_latest_checkpoint() {
        return checkpoint_manager->get_latest_checkpoint();
    }

    // 获取最早检查点
    std::optional<Timestamp> get_earliest_checkpoint() {
        return checkpoint_manager->get_earliest_checkpoint();
    }

    // 检查是否存在指定检查点
    bool has_checkpoint(Timestamp timestamp) {
        return checkpoint_manager->has_checkpoint(timestamp);
    }

    // 获取检查点总数
    size_t get_checkpoint_count() {
        return checkpoint_manager->get_checkpoint_count();
    }

    // 清空所有检查点
    void clear_all_checkpoints() {
        checkpoint_manager->clear_all_checkpoints();
    }

    // 批量插入检查点
    bool batch_insert_checkpoints(const std::vector<Timestamp>& timestamps) {
        return checkpoint_manager->batch_insert_checkpoints(timestamps);
    }

    // 共享内存相关接口
    SharedMemoryHandle allocate_shared_memory(size_t size, ResourceType type) {
        return shared_memory_manager->allocate(size, type);
    }

    bool deallocate_shared_memory(const SharedMemoryHandle& handle) {
        return shared_memory_manager->deallocate(handle);
    }

    bool is_shared_memory_unreferenced(const SharedMemoryHandle& handle) {
        return shared_memory_manager->get_reference_count(handle) == 0;
    }

    // 获取共享内存引用计数
    int get_shared_memory_reference_count(const SharedMemoryHandle& handle) {
        return shared_memory_manager->get_reference_count(handle);
    }

    // 条件释放共享内存（仅当引用计数为0时释放）
    bool deallocate_shared_memory_if_unreferenced(const SharedMemoryHandle& handle) {
        if (shared_memory_manager->get_reference_count(handle) == 0) {
            return shared_memory_manager->deallocate(handle);
        }
        return false;  // 还有引用，不释放
    }

    // 缓存管理接口
    bool enable_resource_cache(ResourceType type, bool enabled) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        if (!enabled) {
            auto it = resource_caches.find(type);
            if (it != resource_caches.end()) {
                it->second->clear();
            }
        }
        return config.set_resource_enabled(type, enabled);
    }

    CacheStatistics get_cache_statistics() const {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        CacheStatistics stats;
        for (const auto& [type, cache] : resource_caches) {
            stats.add_resource_stats(type, cache->get_statistics());
        }
        return stats;
    }

private:
    TimelineCache* get_or_create_cache(ResourceType type) {
        auto it = resource_caches.find(type);
        if (it == resource_caches.end()) {
            auto cache = std::make_unique<TimelineCache>(config.get_cache_config(type), node_pool);
            resource_caches[type] = std::move(cache);
            return resource_caches[type].get();
        }
        return it->second.get();
    }

    UnifiedDataPacket convert_to_unified_packet(ResourceType type, Timestamp timestamp, const RawResourceData& raw_data) {
        // 根据资源类型和数据大小,决定是否使用共享内存存储
        // 并将原始数据转换为 UnifiedDataPacket
    }

    void notify_persistence_required(ResourceType type) {
        // 通知 PersistenceEngine 需要进行数据持久化
        // 可以通过回调或事件机制实现
    }

    // 检查指定范围是否完全被缓存覆盖
    bool is_range_completely_covered_by_cache(ResourceType type, Timestamp start_ts, Timestamp end_ts)；

    // 合并缓存查询结果和持久化查询结果
    QueryResult merge_query_results(const QueryResult& cache_result, 
                                   const QueryResult& persisted_result)；

};

```

```c++
// 伪代码: TimelineCache 类定义
class TimelineCache {
private:
    CacheNode* root;                    // 树的根节点

    CacheNode* list_head;               // 链表的头节点 (时间戳最小的节点)
    CacheNode* list_tail;               // 链表的尾节点 (时间戳最大的节点)

    long long current_size;             // 缓存中元素的数量
    long long current_memory_usage;     // 缓存占用的总内存大小

    CacheNodePool& node_pool;           // 引用外部内存池

    // 用于看要不要访问持久化数据
    long long min_timestamp;
    long long 
    
    // copy-on-write相关字段
    std::atomic<CacheNode*> root_for_read{nullptr};   // 读操作使用的根节点
    CacheNode* root_for_write{nullptr};              // 写操作使用的根节点
    std::atomic<int> active_readers{0};               // 活跃读者计数
    std::atomic_flag write_lock = ATOMIC_FLAG_INIT;   // 写操作锁

    // --- 私有辅助函数 ---
    // (例如: 旋转, 平衡, 递归插入/删除等)
    CacheNode* rotate_left(CacheNode* node);
    CacheNode* rotate_right(CacheNode* node);
    CacheNode* rebalance(CacheNode* node);
    CacheNode* insert(CacheNode* node, long long timestamp, UnifiedDataPacket packet);
    CacheNode* remove(CacheNode* node, long long timestamp);
    void query_range(CacheNode* node, long long start_ts, long long end_ts, std::vector<UnifiedDataPacket>& results);
    int get_height(CacheNode* node);
    int get_balance_factor(CacheNode* node);

    // COW辅助函数
    CacheNode* clone_node_if_shared(CacheNode* node);
    void increment_ref_count(CacheNode* node);
    void decrement_ref_count(CacheNode* node);

public:
    // --- 公共接口 ---
        // 构造函数：接受外部内存池的引用
    explicit TimelineCache(CacheNodePool& pool) : 
        root(nullptr), list_head(nullptr), list_tail(nullptr),
        current_size(0), current_memory_usage(0), 
        min_timestamp(LLONG_MAX), max_timestamp(LLONG_MIN),
        node_pool(pool) {}
    ~TimelineCache(); // 需要实现以释放所有节点

    void insert(long long timestamp, UnifiedDataPacket packet);
    bool remove(long long timestamp);
    UnifiedDataPacket find(long long timestamp);
    std::vector<UnifiedDataPacket> query_range(long long start_ts, long long end_ts);

    // 淘汰策略接口
    UnifiedDataPacket evict_oldest(); // O(1)定位 + O(log N)删除
    UnifiedDataPacket evict_newest(); // O(1)定位 + O(log N)删除

    // 用于看要不要访问持久化数据
    long long get_min_timestamp() const { return min_timestamp; }
    long long get_max_timestamp() const { return max_timestamp; }

    long long size();
    long long memory_usage();
};
```

### 2 核心操作伪代码

#### 插入
``` c++
// 伪代码: 插入操作
void TimelineCache::insert(long long timestamp, UnifiedDataPacket packet) {
    // 1. 创建新节点，但为了优化效率不直接在这里allocate！要用这个内存池管理
    CacheNode* new_node = node_pool.allocate();

    // 2. 如果树为空，新节点既是根也是链表的头和尾
    if (root == nullptr) {
        root = new_node;
        list_head = new_node;
        list_tail = new_node;
        current_size = 1;
        // update memory_usage...
        return;
    }

    // 3. 递归插入到树中 (这是标准的AVL/BST插入逻辑)
    //    这个过程会找到正确的插入位置，并返回新的根节点(如果发生旋转)
    //    在递归插入的辅助函数中，我们需要找到新节点的链表邻居
    //    (in-order predecessor and successor)
    //
    //    简化版逻辑：在找到插入位置的父节点 `parent` 后...
       if (timestamp < parent->timestamp) {
           parent->left = new_node;
           // 链接链表
           new_node->next_by_time = parent;
           new_node->prev_by_time = parent->prev_by_time;
           if (parent->prev_by_time) {
               parent->prev_by_time->next_by_time = new_node;
           }
           parent->prev_by_time = new_node;
           // 如果父节点是头，更新头
           if (list_head == parent) {
               list_head = new_node;
           }
       } else {
           parent->right = new_node;
           // 链接链表
           new_node->prev_by_time = parent;
           new_node->next_by_time = parent->next_by_time;
           if (parent->next_by_time) {
               parent->next_by_time->prev_by_time = new_node;
           }
           parent->next_by_time = new_node;
           // 如果父节点是尾，更新尾
           if (list_tail == parent) {
               list_tail = new_node;
           }
       }
       new_node->parent = parent;

    // 4. 从插入点开始，向上回溯，更新高度并进行平衡操作 (AVL旋转)
    //    rebalance(parent);

    // 5. 更新缓存大小和内存占用
    current_size++;
    // update memory_usage...
}

//伪代码：含锁操作示例
void TimelineCache::insert(long long timestamp, UnifiedDataPacket packet) {
    // 等待所有读操作完成
    while (active_readers.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
    
    // 获取写锁
    while (write_lock.test_and_set(std::memory_order_acquire));
    
    // 如果树为空，直接创建新节点
    if (root_for_write == nullptr) {
        CacheNode* new_node = node_pool.allocate();
        new_node->timestamp = timestamp;
        new_node->packet = std::move(packet);
        new_node->parent = nullptr;
        new_node->left = nullptr;
        new_node->right = nullptr;
        new_node->height = 1;
        new_node->prev_by_time = nullptr;
        new_node->next_by_time = nullptr;
        new_node->ref_count.store(1);
        new_node->is_shared = false;
        
        root_for_write = new_node;
        list_head = new_node;
        list_tail = new_node;
        current_size = 1;
        
        // 原子更新读根节点
        root_for_read.store(root_for_write, std::memory_order_release);
        write_lock.clear(std::memory_order_release);
        return;
    }
    
    // 执行COW插入
    root_for_write = insert_cow(root_for_write, timestamp, std::move(packet));
    
    // 原子更新读根节点
    root_for_read.store(root_for_write, std::memory_order_release);
    
    // 释放写锁
    write_lock.clear(std::memory_order_release);
}

CacheNode* TimelineCache::insert_cow(CacheNode* node, long long timestamp, UnifiedDataPacket packet) {
    // 如果节点为空，创建新节点
    if (node == nullptr) {
        CacheNode* new_node = node_pool.allocate();
        new_node->timestamp = timestamp;
        new_node->packet = std::move(packet);
        new_node->parent = nullptr;
        new_node->left = nullptr;
        new_node->right = nullptr;
        new_node->height = 1;
        new_node->ref_count.store(1);
        new_node->is_shared = false;
        current_size++;
        return new_node;
    }
    
    // COW: 如果节点被共享，则克隆一份
    CacheNode* current_node = clone_node_if_shared(node);
    
    if (timestamp < current_node->timestamp) {
        current_node->left = insert_cow(current_node->left, timestamp, std::move(packet));
        if (current_node->left) {
            current_node->left->parent = current_node;
        }
        
        // 更新链表连接（简化版）
        if (current_node->left && current_node->left->timestamp == timestamp) {
            // 新插入的节点，更新链表
            current_node->left->next_by_time = current_node;
            current_node->left->prev_by_time = current_node->prev_by_time;
            if (current_node->prev_by_time) {
                current_node->prev_by_time->next_by_time = current_node->left;
            }
            current_node->prev_by_time = current_node->left;
            
            if (list_head == current_node) {
                list_head = current_node->left;
            }
        }
    } else if (timestamp > current_node->timestamp) {
        current_node->right = insert_cow(current_node->right, timestamp, std::move(packet));
        if (current_node->right) {
            current_node->right->parent = current_node;
        }
        
        // 更新链表连接（简化版）
        if (current_node->right && current_node->right->timestamp == timestamp) {
            // 新插入的节点，更新链表
            current_node->right->prev_by_time = current_node;
            current_node->right->next_by_time = current_node->next_by_time;
            if (current_node->next_by_time) {
                current_node->next_by_time->prev_by_time = current_node->right;
            }
            current_node->next_by_time = current_node->right;
            
            if (list_tail == current_node) {
                list_tail = current_node->right;
            }
        }
    } else {
        // 相同时间戳，更新数据
        current_node->packet = std::move(packet);
        return current_node;
    }
    
    // 更新高度并进行平衡操作
    current_node->height = 1 + std::max(get_height(current_node->left), get_height(current_node->right));
    current_node = rebalance(current_node);
    
    return current_node;
}

// COW辅助函数
CacheNode* TimelineCache::clone_node_if_shared(CacheNode* node) {
    if (node == nullptr || !node->is_shared) {
        return node;
    }
    
    // 克隆节点
    CacheNode* cloned = node_pool.allocate();
    cloned->timestamp = node->timestamp;
    cloned->packet = node->packet;  // 浅拷贝数据包
    cloned->left = node->left;
    cloned->right = node->right;
    cloned->parent = node->parent;
    cloned->height = node->height;
    cloned->prev_by_time = node->prev_by_time;
    cloned->next_by_time = node->next_by_time;
    cloned->ref_count.store(1);
    cloned->is_shared = false;
    
    // 增加子节点的引用计数
    if (cloned->left) increment_ref_count(cloned->left);
    if (cloned->right) increment_ref_count(cloned->right);
    
    // 减少原节点的引用计数
    decrement_ref_count(node);
    
    return cloned;
}

```

#### 删除
``` c++
// 伪代码: 删除操作
bool TimelineCache::remove(long long timestamp) {
    // 1. 在树中查找要删除的节点 (O(log N))
    CacheNode* node_to_remove = find_node_recursive(root, timestamp);
    if (node_to_remove == nullptr) {
        return false; // 节点不存在
    }

    // 2. 从双向链表中解开此节点 (O(1))
    if (node_to_remove->prev_by_time) {
        node_to_remove->prev_by_time->next_by_time = node_to_remove->next_by_time;
    }
    if (node_to_remove->next_by_time) {
        node_to_remove->next_by_time->prev_by_time = node_to_remove->prev_by_time;
    }

    // 3. 更新链表的头/尾指针 (如果需要)
    if (list_head == node_to_remove) {
        list_head = node_to_remove->next_by_time;
    }
    if (list_tail == node_to_remove) {
        list_tail = node_to_remove->prev_by_time;
    }

    // 4. 从树中删除此节点 (标准的AVL删除逻辑)
    //    这包括处理 0, 1, 2 个子节点的情况，以及后续的向上回溯和再平衡。
    //    这是整个实现中最复杂的部分。
    root = remove_recursive(root, timestamp);

    // 5. 释放节点内存，同样用内存池管理，内部还要处理共享内存逻辑
    node_pool.deallocate(node_to_remove);

    // 6. 检查并处理共享内存引用计数
    if (should_check_shared_memory) {
        handle_shared_memory_reference_on_remove(shared_handle);
    }

    // 7. 更新大小和内存占用
    current_size--;
    // update memory_usage...
    return true;
}

void TimelineCache::handle_shared_memory_reference_on_remove(const SharedMemoryHandle& handle) {
    if (!handle.is_valid()) {
        return;
    }

    // 减少引用计数（这个操作在SharedMemoryManager内部处理）
    // 然后检查是否需要释放
    if (shared_memory_manager->get_reference_count(handle) == 0) {
        bool deallocated = shared_memory_manager->deallocate(handle);
        if (deallocated) {
            log_debug("Shared memory automatically deallocated after reference count reached 0");
        } else {
            log_warning("Failed to deallocate unreferenced shared memory");
        }
    }
}
```

#### 范围删除

``` c++
void remove_range(ResourceType type, long long start_ts, long long end_ts) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex);
    
    auto it = resource_caches.find(type);
    if (it != resource_caches.end()) {
        it->second->remove_range(start_ts, end_ts);
    }

    batch_handle_shared_memory_references_on_remove(handles_to_check);
}

// 批量处理共享内存引用
void TimelineCache::batch_handle_shared_memory_references_on_remove(
    const std::vector<SharedMemoryHandle>& handles) {
    
    std::map<SharedMemoryHandle, int> handle_counts;
    
    // 统计每个句柄的出现次数
    for (const auto& handle : handles) {
        handle_counts[handle]++;
    }
    
    // 检查每个句柄的引用计数并决定是否释放
    for (const auto& [handle, removed_count] : handle_counts) {
        if (!handle.is_valid()) continue;
        
        int current_ref_count = shared_memory_manager->get_reference_count(handle);
        
        // 如果引用计数减去即将删除的数量后为0，则释放
        if (current_ref_count - removed_count <= 0) {
            bool deallocated = shared_memory_manager->deallocate(handle);
            if (deallocated) {
                log_debug("Batch deallocated shared memory after range removal");
            } else {
                log_warning("Failed to batch deallocate shared memory");
            }
        }
    }
    }
```

#### 范围查询
``` c++
// 伪代码: 范围查询(基础递归版，含锁操作示例)
std::vector<UnifiedDataPacket> TimelineCache::query_range(long long start_ts, long long end_ts) {
    // 增加活跃读者计数
    active_readers.fetch_add(1, std::memory_order_acquire);
    
    // 获取读操作的根节点快照
    CacheNode* read_root = root_for_read.load(std::memory_order_acquire);
    
    std::vector<UnifiedDataPacket> results;
    if (read_root != nullptr) {
        query_range_recursive(read_root, start_ts, end_ts, results);
    }
    
    // 减少活跃读者计数
    active_readers.fetch_sub(1, std::memory_order_release);
    
    return results;
}

void TimelineCache::query_range_recursive(CacheNode* node, long long start_ts, long long end_ts, std::vector<UnifiedDataPacket>& results) {
    if (node == nullptr) {
        return;
    }

    // 如果当前节点的时间戳大于范围上限，我们只需要搜索左子树
    if (node->timestamp > end_ts) {
        query_range_recursive(node->left, start_ts, end_ts, results);
    }
    // 如果当前节点的时间戳小于范围下限，我们只需要搜索右子树
    else if (node->timestamp < start_ts) {
        query_range_recursive(node->right, start_ts, end_ts, results);
    }
    // 如果当前节点在范围内，则将其加入结果集，并搜索左右两个子树
    else {
        results.push_back(node->packet);
        query_range_recursive(node->left, start_ts, end_ts, results);
        query_range_recursive(node->right, start_ts, end_ts, results);
    }
}
```
递归版的问题在于当数据量很大时容易造成栈溢出，同时函数调用的开销也会影响性能。特别是在深度较大的AVL树中，递归深度可能达到数千层，每次函数调用都需要保存现场、传递参数和分配栈帧，这些开销累积起来会显著影响查询效率。而非递归版本使用显式栈来模拟递归过程，不仅避免了栈溢出的风险，还能更好地控制内存使用。
``` c++
// 优化版本的非递归范围查询
std::vector<UnifiedDataPacket> TimelineCache::query_range_optimized(long long start_ts, long long end_ts) {
    std::vector<UnifiedDataPacket> results;
    
    if (root == nullptr || start_ts > end_ts) {
        return results;
    }
    
    std::stack<CacheNode*> node_stack;
    CacheNode* current = root;
    
    // 第一阶段：定位到第一个可能在范围内的节点
    // 跳过所有时间戳小于start_ts的节点，直接定位到起始范围
    while (current != nullptr) {
        if (current->timestamp >= start_ts) {
            // 当前节点可能在范围内，先入栈，再检查左子树
            node_stack.push(current);
            current = current->left;
        } else {
            // 当前节点太小，左子树肯定都小于start_ts，直接去右子树
            current = current->right;
        }
    }
    
    // 第二阶段：中序遍历收集范围内的所有节点
    while (!node_stack.empty()) {
        current = node_stack.top();
        node_stack.pop();
        
        // 如果当前节点超出范围上限，停止遍历
        // 因为后续节点的时间戳只会更大
        if (current->timestamp > end_ts) {
            break;
        }
        
        // 当前节点在范围内，加入结果
        if (current->timestamp >= start_ts) {
            results.push_back(current->packet);
        }
        
        // 处理右子树：只处理可能在范围内的部分
        current = current->right;
        while (current != nullptr) {
            if (current->timestamp <= end_ts) {
                // 右子树节点可能在范围内，入栈继续处理
                node_stack.push(current);
                current = current->left;
            } else {
                // 当前节点已经超出范围，右子树所有节点都会超出，跳过
                break;
            }
        }
    }
    
    return results;
}

// 进一步优化：使用链表进行顺序访问
std::vector<UnifiedDataPacket> TimelineCache::query_range_by_list(long long start_ts, long long end_ts) {
    std::vector<UnifiedDataPacket> results;
    
    if (timeline_head == nullptr || start_ts > end_ts) {
        return results;
    }
    
    // 在链表中找到第一个 >= start_ts 的节点
    CacheNode* current = timeline_head;
    while (current != nullptr && current->timestamp < start_ts) {
        current = current->next_by_time;
    }
    
    // 顺序收集范围内的所有节点
    while (current != nullptr && current->timestamp <= end_ts) {
        results.push_back(current->packet);
        current = current->next_by_time;
    }
    
    return results;
}

// 混合查询：根据范围大小选择最优策略
std::vector<UnifiedDataPacket> TimelineCache::query_range_hybrid(long long start_ts, long long end_ts) {
    std::vector<UnifiedDataPacket> results;
    
    if (root == nullptr || start_ts > end_ts) {
        return results;
    }
    
    // 估算查询范围的大小
    long long range_size = end_ts - start_ts;
    long long total_range = max_timestamp - min_timestamp;
    
    // 如果查询范围较小且连续性好，使用链表遍历
    if (range_size * 10 < total_range && timeline_head != nullptr) {
        return query_range_by_list(start_ts, end_ts);
    } else {
        // 否则使用优化的树遍历
        return query_range_optimized(start_ts, end_ts);
    }
}

```

#### 去除最老的数据

``` c++
// 伪代码: 淘汰最老的数据包
UnifiedDataPacket TimelineCache::evict_oldest() {
    // 1. 检查缓存是否为空
    if (list_head == nullptr) {
        throw std::runtime_error("Cache is empty");
    }

    // 2. O(1) 定位到最老的节点
    CacheNode* oldest_node = list_head;
    long long timestamp_to_remove = oldest_node->timestamp;
    UnifiedDataPacket packet_to_return = oldest_node->packet;

    // 3. 调用通用的删除函数，它会处理树和链表的双重解链
    //    这个操作是 O(log N)
    remove(timestamp_to_remove);

    // 4. 返回数据包
    return packet_to_return;
}
```

#### 内存池具体实现
``` c++
class CacheNodePool {
private:
    struct PoolChunk {
        static const size_t CHUNK_SIZE = 1024;
        CacheNode nodes[CHUNK_SIZE];
        PoolChunk* next;
        
        PoolChunk() : next(nullptr) {
            // 构造函数中不需要初始化nodes，因为会在使用时重置
        }
    };
    
    PoolChunk* chunks;                    // 链表头，指向所有chunk
    std::stack<CacheNode*> free_nodes;   // 空闲节点栈
    std::mutex pool_mutex;                // 线程安全锁
    size_t total_allocated;               // 统计已分配节点数

public:
    CacheNodePool() : chunks(nullptr), total_allocated(0) {
        expand_pool();  // 初始化时创建第一个chunk
    }
    
    ~CacheNodePool() {
        // 释放所有chunks
        while (chunks) {
            PoolChunk* to_delete = chunks;
            chunks = chunks->next;
            delete to_delete;
        }
    }
    
    // 分配一个节点
    CacheNode* allocate() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // 如果没有空闲节点，扩展内存池
        if (free_nodes.empty()) {
            expand_pool();
        }
        
        // 从栈顶取出一个空闲节点
        CacheNode* node = free_nodes.top();
        free_nodes.pop();
        total_allocated++;
        
        return node;
    }
    
    // 释放一个节点
    void deallocate(CacheNode* node) {
        if (node == nullptr) return;
        
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // 重置节点状态（清零所有字段）
        memset(node, 0, sizeof(CacheNode));
        
        // 放回空闲栈
        free_nodes.push(node);
        total_allocated--;
    }
    
    // 获取统计信息
    size_t get_allocated_count() const {
        std::lock_guard<std::mutex> lock(pool_mutex);
        return total_allocated;
    }
    
    size_t get_available_count() const {
        std::lock_guard<std::mutex> lock(pool_mutex);
        return free_nodes.size();
    }

private:
    // 扩展内存池
    void expand_pool() {
        // 创建新的chunk
        PoolChunk* new_chunk = new PoolChunk();
        new_chunk->next = chunks;
        chunks = new_chunk;
        
        // 将新chunk中的所有节点加入空闲栈
        for (size_t i = 0; i < PoolChunk::CHUNK_SIZE; i++) {
            free_nodes.push(&new_chunk->nodes[i]);
        }
    }
};
```

#### DataIngestionManager
DataIngestionManager向各个Raw的数据源暴露了向mulitresourceTimelinecache注入数据的接口，同时组合一个TriggerManager提供trigger创建checkpoint的功能。后续TriggerManager这里可以考虑设计为策略模式等等，便于扩展trigger类型和数据类型.
如果要插入的数据是要使用共享内存的类型，就在这里调用mulitresourceTimelinecache暴露出来的接口来创建共享内存node
``` c++
// 批量注入结果
// 修改后的统计信息结构
struct IngestionStats {
    std::map<DataType, uint64_t> packets_per_datatype;
    long long last_update_time;
};

// 检查点数据结构
struct CheckpointData {
    long long reference_timestamp;
    DataType original_data_type;
    std::string checkpoint_type;
    std::vector<uint8_t> state_snapshot;
};

class DataIngestionManager {
private:
    MultiResourceTimelineCache* cache;
    std::unique_ptr<TriggerManager> trigger_manager;
    std::map<DataType, std::atomic<uint64_t>> ingestion_stats;
    mutable std::mutex stats_mutex;
    
    bool use_shared_memory;
    std::map<DataType, size_t> default_shared_memory_sizes;  // 每种数据类型的默认共享内存大小

public:
    DataIngestionManager(MultiResourceTimelineCache* c, bool enable_shared_memory = true) 
        : cache(c), use_shared_memory(enable_shared_memory);

    // 主要数据注入接口 - 支持共享内存和普通内存两种模式
    bool ingest_data(const UnifiedDataPacket& packet) {
        // 1. 验证数据包
        if (!validate_unified_data_packet(packet)) {
            log_error("Invalid unified data packet for data type: " + 
                     std::to_string(static_cast<int>(packet.type)));
            return false;
        }

        try {
            UnifiedDataPacket processed_packet = packet;
            
            // 2. 如果启用共享内存，则分配共享内存并拷贝数据
            if (use_shared_memory) {
                if (!allocate_and_copy_to_shared_memory(processed_packet)) {
                    log_error("Failed to allocate shared memory for data type: " + 
                             std::to_string(static_cast<int>(packet.type)));
                    return false;
                }
            }
            
            // 3. 转换为对应的ResourceType并插入数据到缓存
            ResourceType resource_type = convert_datatype_to_resourcetype(processed_packet.type);
            bool insert_success = cache->insert(resource_type, processed_packet);
            
            if (insert_success) {
                // 4. 更新统计信息
                update_ingestion_stats(processed_packet.type);
                
                // 5. 检查是否需要创建检查点
                trigger_manager->check_and_create_checkpoint(processed_packet);
                
                return true;
            } else {
                // 插入失败时，如果分配了共享内存，需要释放,以及错误处理
                ...
            }
            
        } catch (const std::exception& e) {
            log_error("Exception during data ingestion: " + std::string(e.what()));
            return false;
        }
    }

    // 带有自定义共享内存大小的数据注入接口
    bool ingest_data_with_shared_memory(const UnifiedDataPacket& packet, size_t shared_memory_size) {
        if (!use_shared_memory) {
            return ingest_data(packet);  // 回退到普通模式
        }

        // 验证数据包
        if (!validate_unified_data_packet(packet)) {
            log_error("Invalid unified data packet for data type: " + 
                     std::to_string(static_cast<int>(packet.type)));
            return false;
        }

        try {
            UnifiedDataPacket processed_packet = packet;
            
            // 使用指定大小分配共享内存
            if (!allocate_and_copy_to_shared_memory(processed_packet, shared_memory_size)) {
                log_error("Failed to allocate shared memory with size " + 
                         std::to_string(shared_memory_size) + " for data type: " + 
                         std::to_string(static_cast<int>(packet.type)));
                return false;
            }
            
            // 后续处理与主接口相同
            ResourceType resource_type = convert_datatype_to_resourcetype(processed_packet.type);
            bool insert_success = cache->insert(resource_type, processed_packet);
            
            if (insert_success) {
                update_ingestion_stats(processed_packet.type);
                trigger_manager->check_and_create_checkpoint(processed_packet);
                return true;
            } else {
                // 插入失败时释放共享内存
                cache->deallocate_shared_memory(processed_packet.shared_memory_handle);
                log_error("Failed to insert data packet to cache");
                return false;
            }
            
        } catch (const std::exception& e) {
            log_error("Exception during shared memory data ingestion: " + std::string(e.what()));
            return false;
        }
    }

    // 批量数据注入接口 - 支持共享内存
    BatchIngestionResult ingest_batch_data(const std::vector<UnifiedDataPacket>& packets) {
        BatchIngestionResult result;
        result.total_packets = packets.size();
        result.successful_packets = 0;
        result.failed_packets = 0;

        for (const auto& packet : packets) {
            if (ingest_data(packet)) {
                result.successful_packets++;
            } else {
                result.failed_packets++;
                result.failed_packet_timestamps.push_back(packet.timestamp);
            }
        }

        return result;
    }

    // 零拷贝数据注入接口 - 直接使用已分配的共享内存
    bool ingest_data_zero_copy(UnifiedDataPacket& packet, const SharedMemoryHandle& handle) {
        if (!use_shared_memory) {
            log_error("Zero-copy ingestion requires shared memory to be enabled");
            return false;
        }

        // 验证共享内存句柄
        if (!handle.is_valid()) {
            log_error("Invalid shared memory handle for zero-copy ingestion");
            return false;
        }

        try {
            // 直接使用提供的共享内存句柄
            packet.shared_memory_handle = handle;
            packet.data_ptr = handle.get_ptr();
            packet.data_size = handle.get_size();

            // 验证数据包
            if (!validate_unified_data_packet(packet)) {
                log_error("Invalid unified data packet for zero-copy ingestion");
                return false;
            }

            // 插入到缓存
            ResourceType resource_type = convert_datatype_to_resourcetype(packet.type);
            bool insert_success = cache->insert(resource_type, packet);
            
            if (insert_success) {
                update_ingestion_stats(packet.type);
                trigger_manager->check_and_create_checkpoint(packet);
                return true;
            } else {
                log_error("Failed to insert zero-copy data packet to cache");
                return false;
            }
            
        } catch (const std::exception& e) {
            log_error("Exception during zero-copy data ingestion: " + std::string(e.what()));
            return false;
        }
    }

    // 设置/获取共享内存使用状态
    void set_shared_memory_enabled(bool enabled) {
        use_shared_memory = enabled;
    }

    bool is_shared_memory_enabled() const {
        return use_shared_memory;
    }

    // 设置特定数据类型的默认共享内存大小
    void set_default_shared_memory_size(DataType type, size_t size) {
        default_shared_memory_sizes[type] = size;
    }

    size_t get_default_shared_memory_size(DataType type) const {
        auto it = default_shared_memory_sizes.find(type);
        return (it != default_shared_memory_sizes.end()) ? it->second : 0;
    }

    // 获取触发器管理器的引用，用于配置触发器
    TriggerManager* get_trigger_manager() {
        return trigger_manager.get();
    }

    // 获取数据注入统计信息
    IngestionStats get_ingestion_stats() const {
        std::lock_guard<std::mutex> lock(stats_mutex);
        IngestionStats stats;
        
        for (const auto& [type, count] : ingestion_stats) {
            stats.packets_per_datatype[type] = count.load();
        }
        
        return stats;
    }

    // 配置预设触发器,初始化时调用
    void setup_default_triggers() {
        auto* tm = get_trigger_manager();

        // 为GPS设置时间间隔触发器（每5秒创建一个检查点）
        TriggerManager::TriggerConfig gps_time_trigger;
        gps_time_trigger.type = TriggerManager::TriggerType::TIME_INTERVAL;
        gps_time_trigger.data_type = DataType::GPS;
        gps_time_trigger.interval = std::chrono::milliseconds(5000);
        gps_time_trigger.enabled = true;
        tm->register_trigger(gps_time_trigger);

        // 为IMU设置阈值触发器（加速度超过阈值时创建检查点）
        TriggerManager::TriggerConfig imu_threshold_trigger;
        imu_threshold_trigger.type = TriggerManager::TriggerType::DATA_THRESHOLD;
        imu_threshold_trigger.data_type = DataType::IMU;
        imu_threshold_trigger.threshold_value = 10.0;  // 10m/s² 加速度阈值
        imu_threshold_trigger.enabled = true;
        tm->register_trigger(imu_threshold_trigger);

        // 为相机设置事件检测触发器
        TriggerManager::TriggerConfig camera_event_trigger;
        camera_event_trigger.type = TriggerManager::TriggerType::EVENT_DETECTION;
        camera_event_trigger.data_type = DataType::CAMERA;
        camera_event_trigger.detection_func = [](const UnifiedDataPacket& packet) {
            return detect_important_object_in_shared_memory(packet.data_ptr, packet.data_size);
        };
        camera_event_trigger.enabled = true;
        tm->register_trigger(camera_event_trigger);
    }

private:
    // 初始化各数据类型的默认共享内存大小
    void initialize_shared_memory_sizes() {
        // 根据不同数据类型设置合理的默认大小
        default_shared_memory_sizes[DataType::CAMERA] = 2 * 1024 * 1024;      // 2MB for camera data
        default_shared_memory_sizes[DataType::AUDIO] = 64 * 1024;             // 64KB for audio data
        default_shared_memory_sizes[DataType::GPS] = 1024;                    // 1KB for GPS data
        default_shared_memory_sizes[DataType::IMU] = 512;                     // 512B for IMU data
        default_shared_memory_sizes[DataType::VEHICLE_SIGNAL] = 4 * 1024;     // 4KB for vehicle signals
    }

    // 分配共享内存并拷贝数据
    bool allocate_and_copy_to_shared_memory(UnifiedDataPacket& packet, 
                                           std::optional<size_t> custom_size = std::nullopt) {
        // 确定需要分配的内存大小
        size_t alloc_size = custom_size.value_or(
            std::max(packet.data_size, get_default_shared_memory_size(packet.type))
        );

        if (alloc_size == 0) {
            log_error("Cannot allocate zero-size shared memory");
            return false;
        }

        try {
            // 分配共享内存
            ResourceType resource_type = convert_datatype_to_resourcetype(packet.type);
            SharedMemoryHandle handle = cache->allocate_shared_memory(alloc_size, resource_type);
            
            if (!handle.is_valid()) {
                log_error("Failed to allocate shared memory of size: " + std::to_string(alloc_size));
                return false;
            }

            // 拷贝数据到共享内存
            void* shared_ptr = handle.get_ptr();
            if (shared_ptr && packet.data_ptr && packet.data_size > 0) {
                std::memcpy(shared_ptr, packet.data_ptr, packet.data_size);
            }

            // 更新数据包信息
            packet.shared_memory_handle = handle;
            packet.data_ptr = shared_ptr;
            // 保持原有的 data_size，不改变为 alloc_size

            return true;

        } catch (const std::exception& e) {
            log_error("Exception during shared memory allocation: " + std::string(e.what()));
            return false;
        }
    }

    bool validate_unified_data_packet(const UnifiedDataPacket& packet) {
        // 基本验证
        if (packet.data_ptr == nullptr || packet.data_size == 0) {
            return false;
        }
        
        if (packet.timestamp <= 0) {
            return false;
        }

        // 根据数据类型进行特定验证
        switch (packet.type) {
            case DataType::CAMERA:
                return validate_camera_shared_memory(packet.data_ptr, packet.data_size);
            case DataType::AUDIO:
                return validate_audio_shared_memory(packet.data_ptr, packet.data_size);
            case DataType::GPS:
                return validate_gps_shared_memory(packet.data_ptr, packet.data_size);
            case DataType::IMU:
                return validate_imu_shared_memory(packet.data_ptr, packet.data_size);
            case DataType::VEHICLE_SIGNAL:
                return validate_vehicle_signal_shared_memory(packet.data_ptr, packet.data_size);
            default:
                return false;
        }
    }

    ResourceType convert_datatype_to_resourcetype(DataType data_type) {
        switch (data_type) {
            case DataType::CAMERA: return ResourceType::CAMERA;
            case DataType::AUDIO: return ResourceType::AUDIO;
            case DataType::GPS: return ResourceType::GPS;
            case DataType::IMU: return ResourceType::IMU;
            case DataType::VEHICLE_SIGNAL: return ResourceType::VEHICLE_SIGNAL;
            default: return ResourceType::CAMERA;
        }
    }

    void update_ingestion_stats(DataType type) {
        ingestion_stats[type]++;
    }

    // 特定数据类型的共享内存验证方法
    bool validate_camera_shared_memory(void* data_ptr, size_t data_size);
    bool validate_audio_shared_memory(void* data_ptr, size_t data_size);
    bool validate_gps_shared_memory(void* data_ptr, size_t data_size);
    bool validate_imu_shared_memory(void* data_ptr, size_t data_size);
    bool validate_vehicle_signal_shared_memory(void* data_ptr, size_t data_size);
    
    // 事件检测辅助方法
    bool detect_important_object_in_shared_memory(void* image_data, size_t data_size);
    
    // 日志记录方法
    void log_error(const std::string& message);
};

```

#### Triggermanager
``` c++
class TriggerManager {
public:
    // 触发器类型枚举
    enum class TriggerType {
        TIME_INTERVAL,      // 时间间隔触发
        EVENT_DETECTION,    // 事件检测触发
        DATA_THRESHOLD,     // 数据阈值触发
        MANUAL             // 手动触发
    };

    // 触发器配置结构
    struct TriggerConfig {
        TriggerType type;
        DataType data_type;  // 改为使用 DataType
        std::chrono::milliseconds interval;
        std::function<bool(const UnifiedDataPacket&)> detection_func;  // 使用 UnifiedDataPacket
        double threshold_value;
        bool enabled;
    };

private:
    MultiResourceTimelineCache* cache;
    std::map<DataType, std::vector<TriggerConfig>> triggers;
    std::map<DataType, long long> last_checkpoint_time;
    std::mutex trigger_mutex;

public:
    TriggerManager(MultiResourceTimelineCache* c) : cache(c) {}

    // 注册触发器
    void register_trigger(const TriggerConfig& config) {
        std::lock_guard<std::mutex> lock(trigger_mutex);
        triggers[config.data_type].push_back(config);
    }

    // 检查是否需要创建检查点（数据输入时调用）
    void check_and_create_checkpoint(const UnifiedDataPacket& packet) {
        std::lock_guard<std::mutex> lock(trigger_mutex);
        
        auto trigger_it = triggers.find(packet.type);
        if (trigger_it == triggers.end()) {
            return;
        }

        for (const auto& trigger : trigger_it->second) {
            if (!trigger.enabled) continue;

            bool should_create_checkpoint = false;

            switch (trigger.type) {
                case TriggerType::TIME_INTERVAL:
                    should_create_checkpoint = check_time_interval_trigger(packet, trigger);
                    break;

                case TriggerType::EVENT_DETECTION:
                    should_create_checkpoint = check_event_detection_trigger(packet, trigger);
                    break;

                case TriggerType::DATA_THRESHOLD:
                    should_create_checkpoint = check_threshold_trigger(packet, trigger);
                    break;

                case TriggerType::MANUAL:
                    break;
            }

            if (should_create_checkpoint) {
                create_checkpoint(packet);
                break;
            }
        }
    }

private:
    bool check_time_interval_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
        auto last_time_it = last_checkpoint_time.find(packet.type);
        if (last_time_it == last_checkpoint_time.end()) {
            last_checkpoint_time[packet.type] = packet.timestamp;
            return true;
        }

        auto elapsed = packet.timestamp - last_time_it->second;
        return elapsed >= config.interval.count();
    }

    bool check_event_detection_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
        if (config.detection_func) {
            return config.detection_func(packet);
        }
        return false;
    }

    bool check_threshold_trigger(const UnifiedDataPacket& packet, const TriggerConfig& config) {
        double packet_value = extract_numeric_value(packet);
        return packet_value > config.threshold_value;
    }

    double extract_numeric_value(const UnifiedDataPacket& packet) {
        if (!packet.data_ptr || packet.data_size == 0) {
            return 0.0;
        }

        switch (packet.type) {
            case DataType::GPS: {
                // 假设GPS数据格式包含速度信息
                if (packet.data_size >= sizeof(double)) {
                    double* speed_ptr = static_cast<double*>(packet.data_ptr);
                    return *speed_ptr;
                }
                break;
            }
            case DataType::IMU: {
                // 假设IMU数据格式包含加速度信息
                if (packet.data_size >= 3 * sizeof(double)) {
                    double* accel_ptr = static_cast<double*>(packet.data_ptr);
                    // 计算加速度模长
                    return sqrt(accel_ptr[0]*accel_ptr[0] + accel_ptr[1]*accel_ptr[1] + accel_ptr[2]*accel_ptr[2]);
                }
                break;
            }
            case DataType::VEHICLE_SIGNAL: {
                // 假设车辆信号数据格式
                if (packet.data_size >= sizeof(float)) {
                    float* signal_ptr = static_cast<float*>(packet.data_ptr);
                    return static_cast<double>(*signal_ptr);
                }
                break;
            }
            default:
                return 0.0;
        }
        return 0.0;
    }

    void create_checkpoint(const UnifiedDataPacket& reference_packet) {
        // 直接插入时间戳作为检查点
        bool success = cache->insert_checkpoint(reference_packet.timestamp);
        
        if (success) {
            last_checkpoint_time[reference_packet.type] = reference_packet.timestamp;
            log_info("Created checkpoint at timestamp " + std::to_string(reference_packet.timestamp));
        } else {
            log_error("Failed to create checkpoint at timestamp " + std::to_string(reference_packet.timestamp));
        }
    }

    ResourceType convert_datatype_to_resourcetype(DataType data_type) {
        switch (data_type) {
            case DataType::CAMERA: return ResourceType::CAMERA;
            case DataType::AUDIO: return ResourceType::AUDIO;
            case DataType::GPS: return ResourceType::GPS;
            case DataType::IMU: return ResourceType::IMU;
            case DataType::VEHICLE_SIGNAL: return ResourceType::VEHICLE_SIGNAL;
            default: return ResourceType::CAMERA; // 默认值
        }
    }

    std::vector<uint8_t> serialize_checkpoint_data(const CheckpointData& data);
    std::string datatype_to_string(DataType type);
};

```

#### Query类：额外的前处理/后处理/参数
根据特定数据类型，可能还需要一些特别的前处理/后处理/额外参数等等。现在已知的需求是：1、camera和大模型一些描述文字关联 query的时候可以选择只查询文字 也可以都查询 2、Vehicle Signal要求Explainable：比如存的数据可能是16 2 分别代表摄像头和一个行为，需要有一个方法去配置文件里读到它们的解释然后再翻译再返回给大模型。
该类同时负责解析IPC RPC SQL native接口四类调用。

``` c++
// 统一的内部查询请求格式（native格式）
struct NativeQueryRequest {
    std::string device_type;           // "camera", "vehicle_signal", "gps", "imu"
    std::string query_type;            // "time_range", "timestamp", "event", "pattern"
    std::map<std::string, std::variant<long long, std::string, double>> parameters;
    QueryOptions options;
    
    // 后处理标识
    bool need_post_processing = false;
    std::string post_processing_type;  // "text_only", "explainable", "compress", etc.
    std::map<std::string, std::string> post_processing_params;
};

// 内部查询响应
struct NativeQueryResponse {
    bool success;
    std::string error_message;
    UniversalQueryResult query_result;
};

// 最终响应（经过后处理）
struct FinalQueryResponse {
    bool success;
    std::string error_message;
    std::string response_data;  // 序列化后的数据
    std::map<std::string, std::string> metadata;
};

//接口解析器 - 将四种接口统一转换为native格式
class InterfaceParser {
public:
    // 统一的解析入口
    static std::pair<NativeQueryRequest, bool> parse_request(const QueryRequest& original_request) {
        NativeQueryRequest native_request;
        bool parse_success = false;
        
        switch (original_request.interface_type) {
            case InterfaceType::IPC:
                std::tie(native_request, parse_success) = parse_ipc_request(original_request);
                break;
            case InterfaceType::RPC:
                std::tie(native_request, parse_success) = parse_rpc_request(original_request);
                break;
            case InterfaceType::SQL:
                std::tie(native_request, parse_success) = parse_sql_request(original_request);
                break;
            case InterfaceType::NATIVE:
                std::tie(native_request, parse_success) = parse_native_request(original_request);
                break;
        }
        
        // 解析后处理需求
        if (parse_success) {
            determine_post_processing_needs(native_request);
        }
        
        return {native_request, parse_success};
    }

private:
    static std::pair<NativeQueryRequest, bool> parse_ipc_request(const QueryRequest& request);

    static std::pair<NativeQueryRequest, bool> parse_rpc_request(const QueryRequest& request);

    static std::pair<NativeQueryRequest, bool> parse_sql_request(const QueryRequest& request);

    static std::pair<NativeQueryRequest, bool> parse_native_request(const QueryRequest& request);

    // 确定是否需要后处理, 可以直接在request发起的时候用一个字段来确认，或者扩展为更丰富的规则等等，现在是一个示例。确认之后在格式化的请求中标注。
    static void determine_post_processing_needs(NativeQueryRequest& native_req) {
        // 规则1: camera + text_only参数 -> 需要文本后处理
        if (native_req.device_type == "camera") {
            auto text_only_it = native_req.parameters.find("text_only");
            if (text_only_it != native_req.parameters.end() && 
                std::get<std::string>(text_only_it->second) == "true") {
                native_req.need_post_processing = true;
                native_req.post_processing_type = "text_only";
            }
        }
        
        // 规则2: vehicle_signal + explainable参数 -> 需要可解释性后处理
        if (native_req.device_type == "vehicle_signal") {
            auto explainable_it = native_req.parameters.find("explainable");
            if (explainable_it != native_req.parameters.end() && 
                std::get<std::string>(explainable_it->second) == "true") {
                native_req.need_post_processing = true;
                native_req.post_processing_type = "explainable";
            }
        }
        
        // 规则3: 根据options判断
        if (native_req.options.require_explanation) {
            native_req.need_post_processing = true;
            if (native_req.post_processing_type.empty()) {
                native_req.post_processing_type = "explainable";
            }
        }
    }
    
    static std::vector<std::string> split_string(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            result.push_back(item);
        }
        return result;
    }
    
    static std::variant<long long, std::string, double> parse_value(const std::string& value) {
        // 尝试解析为数字
        try {
            if (value.find('.') != std::string::npos) {
                return std::stod(value);
            } else {
                return std::stoll(value);
            }
        } catch (...) {
            return value;  // 作为字符串返回
        }
    }
};

//查询结果后处理器
class QueryDataProcessor {
private:
    std::map<std::string, std::string> config_mappings;
    std::unique_ptr<ImageAnalyzer> image_analyzer;

public:
    QueryDataProcessor() {
        load_configuration_mappings();
        image_analyzer = std::make_unique<ImageAnalyzer>();
    }

    // 统一的后处理入口
    UniversalQueryResult QueryDataProcessor::process(const UniversalQueryResult& raw_result, 
                                                const std::string& processing_type,
                                                const std::string& device_type,
                                                const std::map<std::string, std::string>& params) {
    UniversalQueryResult processed_result = raw_result;  // 复制原始结果
    
    try {
        // 1. 根据设备类型和处理类型选择处理策略
        if (device_type == "Vehicle" && processing_type == "Explainable") {
            processed_result = process_vehicle_explainable(raw_result, params);
        }
        else if (device_type == "Camera" && processing_type == "ImageAnalysis") {
            processed_result = process_camera_image_analysis(raw_result, params);
        }
        else if (device_type == "Audio" && processing_type == "Transcription") {
            processed_result = process_audio_transcription(raw_result, params);
        }
        else if (processing_type == "Compression") {
            processed_result = process_data_compression(raw_result, params);
        }
        else if (processing_type == "Format") {
            processed_result = process_format_conversion(raw_result, params);
        }
        else {
            // 默认情况：不进行额外处理，直接返回原始结果
            log_info("No specific processing required for device: " + device_type + 
                    ", processing type: " + processing_type);
        }
        
        // 2. 添加处理元信息
        processed_result.processing_metadata["processed_by"] = "QueryDataProcessor";
        processed_result.processing_metadata["processing_type"] = processing_type;
        processed_result.processing_metadata["device_type"] = device_type;
        processed_result.processing_metadata["processed_at"] = get_current_timestamp_string();
        
        return processed_result;
        
    } catch (const std::exception& e) {
        log_error("Error in process: " + std::string(e.what()));
        // 处理失败时返回原始结果，并添加错误信息
        processed_result.processing_metadata["error"] = e.what();
        return processed_result;
    }
}

private:
    // **核心功能：Vehicle Signal的可解释处理**
    UniversalQueryResult process_vehicle_explainable(const UniversalQueryResult& raw_result, 
                                                     const std::map<std::string, std::string>& params) {
        UniversalQueryResult explainable_result = raw_result;
        
        // 遍历所有Vehicle Signal类型的数据包
        for (auto& [resource_type, packets] : explainable_result.resource_data) {
            if (resource_type == ResourceType::VEHICLE_SIGNAL) {
                for (auto& packet : packets) {
                    // 解析原始的Vehicle Signal数据
                    auto explained_data = explain_vehicle_signal_data(packet);
                    
                    // 替换或扩展原始数据
                    packet.explained_data = explained_data;
                    packet.is_explained = true;
                }
            }
        }
        
        explainable_result.processing_metadata["explanation_applied"] = "true";
        return explainable_result;
    }
    
    // **Vehicle Signal数据解释的核心逻辑**
    ExplainedVehicleData explain_vehicle_signal_data(const UnifiedDataPacket& packet) {
        ExplainedVehicleData explained;
        
        // 1. 从数据包中提取原始数值
        auto raw_values = extract_raw_vehicle_values(packet);
        
        // 2. 逐个解释每个数值
        for (const auto& [field_name, raw_value] : raw_values) {
            ExplanationEntry entry;
            entry.field_name = field_name;
            entry.raw_value = raw_value;
            
            // 3. 根据字段名从配置文件获取解释
            if (field_name == "camera_status") {
                entry.explanation = explain_camera_status(raw_value);
            }
            else if (field_name == "behavior_code") {
                entry.explanation = explain_behavior_code(raw_value);
            }
            else if (field_name == "gear_position") {
                entry.explanation = explain_gear_position(raw_value);
            }
            else if (field_name == "door_status") {
                entry.explanation = explain_door_status(raw_value);
            }
            else {
                // 通用解释逻辑：从配置映射中查找
                entry.explanation = get_explanation_from_config(field_name, raw_value);
            }
            
            explained.explanations.push_back(entry);
        }
        
        // 4. 生成整体解释摘要
        explained.summary = generate_vehicle_status_summary(explained.explanations);
        
        return explained;
    }
    
    // **从配置文件读取解释信息**
    std::string explain_camera_status(int raw_value) {
        // 示例：16 -> "Front Camera Active"
        std::string config_key = "vehicle.camera_status." + std::to_string(raw_value);
        
        auto it = config_mappings.find(config_key);
        if (it != config_mappings.end()) {
            return it->second;
        }
        
        // 默认解释
        switch (raw_value) {
            case 16: return "Front Camera Active";
            case 17: return "Rear Camera Active";
            case 18: return "Left Camera Active";
            case 19: return "Right Camera Active";
            case 0:  return "All Cameras Inactive";
            default: return "Unknown Camera Status (" + std::to_string(raw_value) + ")";
        }
    }
    
    std::string explain_behavior_code(int raw_value) {
        // 示例：2 -> "Lane Change Detected"
        std::string config_key = "vehicle.behavior." + std::to_string(raw_value);
        
        auto it = config_mappings.find(config_key);
        if (it != config_mappings.end()) {
            return it->second;
        }
        
        // 默认解释
        switch (raw_value) {
            case 1: return "Normal Driving";
            case 2: return "Lane Change Detected";
            case 3: return "Sudden Braking";
            case 4: return "Rapid Acceleration";
            case 5: return "Sharp Turn";
            default: return "Unknown Behavior (" + std::to_string(raw_value) + ")";
        }
    }
    
    // **通用配置查找方法**
    std::string get_explanation_from_config(const std::string& field_name, int raw_value) {
        std::string config_key = "vehicle." + field_name + "." + std::to_string(raw_value);
        
        auto it = config_mappings.find(config_key);
        if (it != config_mappings.end()) {
            return it->second;
        }
        
        return "Raw Value: " + std::to_string(raw_value);
    }
    
    // **加载配置映射**
    void load_configuration_mappings() {
        // 从配置文件加载映射关系
        // 实际实现中可以从JSON/XML/INI文件读取
        
        // 摄像头状态映射
        config_mappings["vehicle.camera_status.16"] = "Front Camera Active";
        config_mappings["vehicle.camera_status.17"] = "Rear Camera Active";
        config_mappings["vehicle.camera_status.18"] = "Left Side Camera Active";
        config_mappings["vehicle.camera_status.19"] = "Right Side Camera Active";
        config_mappings["vehicle.camera_status.0"] = "All Cameras Inactive";
        
        // 行为代码映射
        config_mappings["vehicle.behavior.1"] = "Normal Driving";
        config_mappings["vehicle.behavior.2"] = "Lane Change Detected";
        config_mappings["vehicle.behavior.3"] = "Emergency Braking";
        config_mappings["vehicle.behavior.4"] = "Rapid Acceleration";
        config_mappings["vehicle.behavior.5"] = "Sharp Turn Left";
        config_mappings["vehicle.behavior.6"] = "Sharp Turn Right";
        
        // 档位状态映射
        config_mappings["vehicle.gear_position.1"] = "Park";
        config_mappings["vehicle.gear_position.2"] = "Reverse";
        config_mappings["vehicle.gear_position.3"] = "Neutral";
        config_mappings["vehicle.gear_position.4"] = "Drive";
        config_mappings["vehicle.gear_position.5"] = "Sport Mode";
        
        // 门状态映射（位掩码解释）
        config_mappings["vehicle.door_status.0"] = "All Doors Closed";
        config_mappings["vehicle.door_status.1"] = "Driver Door Open";
        config_mappings["vehicle.door_status.2"] = "Passenger Door Open";
        config_mappings["vehicle.door_status.4"] = "Rear Left Door Open";
        config_mappings["vehicle.door_status.8"] = "Rear Right Door Open";
        
        log_info("Loaded " + std::to_string(config_mappings.size()) + " configuration mappings");
    }
    
    // **其他处理类型的伪代码实现**
    UniversalQueryResult process_camera_image_analysis(const UniversalQueryResult& raw_result, 
                                                      const std::map<std::string, std::string>& params) {
        // 图像分析处理
        // 使用image_analyzer进行物体检测、人脸识别等
        return raw_result;  // 伪代码
    }
    
    UniversalQueryResult process_audio_transcription(const UniversalQueryResult& raw_result, 
                                                    const std::map<std::string, std::string>& params) {
        // 音频转录处理
        return raw_result;  // 伪代码
    }
    
    UniversalQueryResult process_data_compression(const UniversalQueryResult& raw_result, 
                                                 const std::map<std::string, std::string>& params) {
        // 数据压缩处理
        return raw_result;  // 伪代码
    }
    
    // **辅助方法**
    std::map<std::string, int> extract_raw_vehicle_values(const UnifiedDataPacket& packet) {
        std::map<std::string, int> values;
        
        // 从packet.data中解析出各个字段的原始数值
        // 这里假设data是按某种格式存储的结构化数据
        if (packet.data.size() >= 8) {  // 假设至少有4个2字节的值
            values["camera_status"] = *reinterpret_cast<const int16_t*>(packet.data.data());
            values["behavior_code"] = *reinterpret_cast<const int16_t*>(packet.data.data() + 2);
            values["gear_position"] = *reinterpret_cast<const int16_t*>(packet.data.data() + 4);
            values["door_status"] = *reinterpret_cast<const int16_t*>(packet.data.data() + 6);
        }
        
        return values;
    }
    
    std::string generate_vehicle_status_summary(const std::vector<ExplanationEntry>& explanations) {
        std::stringstream summary;
        summary << "Vehicle Status Summary: ";
        
        for (const auto& entry : explanations) {
            summary << entry.field_name << "=" << entry.explanation << "; ";
        }
        
        return summary.str();
    }
    
    std::string get_current_timestamp_string() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// **支持数据结构定义**
struct ExplanationEntry {
    std::string field_name;
    int raw_value;
    std::string explanation;
};

struct ExplainedVehicleData {
    std::vector<ExplanationEntry> explanations;
    std::string summary;
};

class BaseQuery {
protected:
    MultiResourceTimelineCache* cache;
    std::unique_ptr<QueryDataProcessor> data_processor;
    
    // 各种接口的响应格式化器
    std::unique_ptr<ResponseFormatter> response_formatter;

public:
    BaseQuery(MultiResourceTimelineCache* c) : cache(c) {
        data_processor = std::make_unique<QueryDataProcessor>();
        response_formatter = std::make_unique<ResponseFormatter>();
    }

    virtual ~BaseQuery() = default;

    // 统一的查询入口
    FinalQueryResponse handle_query(const QueryRequest& original_request) {
        // 步骤1: 解析四种接口格式为统一的native格式
        auto [native_request, parse_success] = InterfaceParser::parse_request(original_request);
        
        if (!parse_success) {
            FinalQueryResponse error_response;
            error_response.success = false;
            error_response.error_message = "Failed to parse request";
            return error_response;
        }

        // 步骤2: 执行统一的查询逻辑
        auto native_response = execute_native_query(native_request);
        
        if (!native_response.success) {
            return convert_to_final_response(native_response, original_request.interface_type);
        }

        // 步骤3: 判断是否需要后处理
        if (native_request.need_post_processing) {
            native_response.query_result = data_processor->process(
                native_response.query_result,
                native_request.post_processing_type,
                native_request.device_type,
                native_request.post_processing_params
            );
        }

        // 步骤4: 格式化响应
        return convert_to_final_response(native_response, original_request.interface_type);
    }

protected:
    // 执行统一的查询逻辑（由子类实现）
    virtual NativeQueryResponse execute_native_query(const NativeQueryRequest& request) = 0;

private:
    FinalQueryResponse convert_to_final_response;
};

class CameraQuery : public BaseQuery {
public:
    CameraQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    NativeQueryResponse execute_native_query(const NativeQueryRequest& request) override {
        NativeQueryResponse response;
        
        try {
            if (request.query_type == "time_range") {
                auto start_time = std::get<long long>(request.parameters.at("start_time"));
                auto end_time = std::get<long long>(request.parameters.at("end_time"));
                
                auto raw_result = cache->query_by_range(ResourceType::CAMERA, start_time, end_time);
                response.query_result = convert_to_universal_result(raw_result, DataType::CAMERA);
                response.success = true;
                
            } else if (request.query_type == "timestamp") {
                auto timestamp = std::get<long long>(request.parameters.at("timestamp"));
                
                auto raw_result = cache->query_by_range(ResourceType::CAMERA, timestamp - 100, timestamp + 100);
                response.query_result = convert_to_universal_result(raw_result, DataType::CAMERA);
                response.success = true;
            }
            
        } catch (const std::exception& e) {
            response.success = false;
            response.error_message = "Camera query error: " + std::string(e.what());
        }
        
        return response;
    }

private:
};

class VehicleSignalQuery : public BaseQuery {
public:
    VehicleSignalQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    NativeQueryResponse execute_native_query(const NativeQueryRequest& request) override {
        NativeQueryResponse response;
        
        try {
            if (request.query_type == "time_range") {
                auto start_time = std::get<long long>(request.parameters.at("start_time"));
                auto end_time = std::get<long long>(request.parameters.at("end_time"));
                
                auto raw_result = cache->query_by_range(ResourceType::VEHICLE_SIGNAL, start_time, end_time);
                response.query_result = convert_to_universal_result(raw_result, DataType::VEHICLE_SIGNAL);
                response.success = true;
            }
            
        } catch (const std::exception& e) {
            response.success = false;
            response.error_message = "Vehicle signal query error: " + std::string(e.what());
        }
        
        return response;
    }
};


```

### 性能对比
| 操作               | std::map + std::deque      | 索引链表树 (本设计)       | 优势                                                                 |
|--------------------|---------------------------|--------------------------|---------------------------------------------------------------------|
| 插入               | O(log N) + O(1)           | O(log N)                 | 单一结构，原子性更好，常数时间更低                                   |
| 删除               | O(log N) + O(N) (最差)     | O(log N)                 | 巨大提升，无需扫描deque                                             |
| 按时间戳查找       | O(log N)                  | O(log N)                 | 相当                                                               |
| 范围查询           | O(log N + K)              | O(log N + K)             | 相当                                                               |
| 淘汰最老数据       | O(1)定位 + O(log N)删除    | O(1)定位 + O(log N)删除   | 逻辑更清晰，单一结构，避免同步问题                                   |
| 内存开销           | 高 (两个容器的开销)        | 低 (仅一个结构体开销)     | 显著节省内存                                                        |