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

#include "CacheNodePool.h"

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

struct UnifiedDataPacketBenchmark {
    long long timestamp;
    DataType type;
    void* dataPtr; // 指向数据
    size_t dataSize;

    UnifiedDataPacketBenchmark() : timestamp(0), type(DataType::CAMERA), dataPtr(nullptr), dataSize(0) {}

    UnifiedDataPacketBenchmark(long long ts, DataType t, const void* ptr, size_t size)
        : timestamp(ts), type(t), dataSize(size) {
        if (ptr && size > 0) {
            dataPtr = malloc(size);
            if (!dataPtr) throw std::bad_alloc();
            memcpy(dataPtr, ptr, size);
        } else {
            dataPtr = nullptr;
        }
    }

    // 拷贝构造 (深拷贝)
    UnifiedDataPacketBenchmark(const UnifiedDataPacketBenchmark& other)
        : timestamp(other.timestamp), type(other.type), dataSize(other.dataSize) {
        if (other.dataPtr && other.dataSize > 0) {
            dataPtr = malloc(other.dataSize);
            if (!dataPtr) throw std::bad_alloc();
            memcpy(dataPtr, other.dataPtr, other.dataSize);
        } else {
            dataPtr = nullptr;
        }
    }

    // 赋值运算符 (深拷贝)
    UnifiedDataPacketBenchmark& operator=(const UnifiedDataPacketBenchmark& other) {
        if (&other == this) return *this;
        if (dataPtr) free(dataPtr);
        timestamp = other.timestamp;
        type = other.type;
        dataSize = other.dataSize;
        if (other.dataPtr && other.dataSize > 0) {
            dataPtr = malloc(other.dataSize);
            if (!dataPtr) throw std::bad_alloc();
            memcpy(dataPtr, other.dataPtr, other.dataSize);
        } else {
            dataPtr = nullptr;
        }
        return *this;
    }

    ~UnifiedDataPacketBenchmark() {
        if (dataPtr) free(dataPtr);
    }
};

// find出来的数据是否会被修改呢？
// 还是只是需要拿到数据？
class TimelineCacheBaseLine {
private:
    map<long long, shared_ptr<UnifiedDataPacketBenchmark>> dataMap;
    deque<long long> dataDeque;

public:
    void insert(long long timestamp, DataType type, void* dataPtr, size_t dataSize);

    shared_ptr<UnifiedDataPacketBenchmark> find(long long timestamp);

    vector<shared_ptr<UnifiedDataPacketBenchmark>> queryRange(long long startTs, long long endTs);

    shared_ptr<UnifiedDataPacketBenchmark> evictOldest();
    
    void insertCameraData(long long timestamp, const char* imageData, size_t imageSize);

    void insertAudioData(long long timestamp, const char* audioData, size_t audioSize);

    void clear();

    ~TimelineCacheBaseLine() {}
};

struct CacheNodeBenchmark {
    // 键与值
    long long timestamp;                // 键: 时间戳, 用于树的排序
    UnifiedDataPacketBenchmark packet;           // 值: 统一数据包

    // --- 树相关指针 ---
    CacheNodeBenchmark* parent;                  // 指向父节点
    CacheNodeBenchmark* left;                    // 指向左子节点
    CacheNodeBenchmark* right;                   // 指向右子节点
    int height;                         // 节点高度 (用于AVL树的平衡)

    // --- 链表相关指针 ---
    CacheNodeBenchmark* prevByTime;            // 指向前一个节点 (按时间戳排序)
    CacheNodeBenchmark* nextByTime;            // 指向后一个节点 (按时间戳排序)

    // --- copy on write相关字段 ---
    std::atomic<int> refCount{1};      // 引用计数，用于COW
    bool isShared{false};              // 标记节点是否被多个版本共享
    
    CacheNodeBenchmark(long long ts, const UnifiedDataPacketBenchmark& p);
};

class CacheNodePoolBenchmark {
public:
    CacheNodeBenchmark* createNode(long long ts, const UnifiedDataPacketBenchmark& p) {
        return new CacheNodeBenchmark(ts, p);
    }
    void destroyNode(CacheNodeBenchmark* n) {
        delete n;
    }
};

class TimelineCacheBenchmark {
private:
    CacheNodeBenchmark* root;
    CacheNodeBenchmark* listHead;
    CacheNodeBenchmark* listTail;

    long long currentSize;
    long long currentMemoryUsage;

    CacheNodePoolBenchmark& nodePool;

    long long minTimestamp;
    long long maxTimestamp;

    // --- AVL 辅助 ---
    static int getHeight(CacheNodeBenchmark* n);
    static int getBalanceFactor(CacheNodeBenchmark* n);
    static void updateHeight(CacheNodeBenchmark* n);

    CacheNodeBenchmark* rotateLeft(CacheNodeBenchmark* x);
    CacheNodeBenchmark* rotateRight(CacheNodeBenchmark* y);
    CacheNodeBenchmark* rebalance(CacheNodeBenchmark* n);

    CacheNodeBenchmark* insert(CacheNodeBenchmark* node, long long ts, const UnifiedDataPacketBenchmark& packet, CacheNodeBenchmark*& inserted);
    CacheNodeBenchmark* removePureAVL(CacheNodeBenchmark* node, long long ts, CacheNodeBenchmark*& deletedNode);

    void queryRange(CacheNodeBenchmark* node, long long l, long long r, std::vector<UnifiedDataPacketBenchmark>& out) const;

    void destroy(CacheNodeBenchmark* node);
    CacheNodeBenchmark* findNode(long long ts) const;

    CacheNodeBenchmark* findPredecessor(long long ts) const;
    CacheNodeBenchmark* findSuccessor(long long ts) const;

    void linkIntoList(CacheNodeBenchmark* node);
    void unlinkFromList(CacheNodeBenchmark* node);
    void relinkIntoList(CacheNodeBenchmark* node);

    void refreshMinMaxAfterChange();

public:
    explicit TimelineCacheBenchmark(CacheNodePoolBenchmark& pool);
    ~TimelineCacheBenchmark();

    void insert(long long timestamp, UnifiedDataPacketBenchmark packet);
    bool remove(long long timestamp);
    UnifiedDataPacketBenchmark find(long long timestamp);
    std::vector<UnifiedDataPacketBenchmark> queryRange(long long startTs, long long endTs);

    UnifiedDataPacketBenchmark evictOldest();
    UnifiedDataPacketBenchmark evictNewest();

    void clear();

    long long getMinTimestamp() const;
    long long getMaxTimestamp() const;

    long long size();
    long long memoryUsage();
};