#include "TimelineCache.h"

#include <atomic>

struct CacheNode {
    long long timestamp;
    UnifiedDataPacket packet;

    // AVL 指针
    CacheNode* parent;
    CacheNode* left;
    CacheNode* right;
    int height;

    // 链表指针
    CacheNode* prev_by_time;
    CacheNode* next_by_time;

    // COW (当前未使用)
    std::atomic<int> ref_count{1};
    bool is_shared{false};

    explicit CacheNode(const UnifiedDataPacket& p);
};

class CacheNodePool {
public:
    // ctor/dtor
    explicit CacheNodePool(size_t nodes_per_slab = 4096);
    ~CacheNodePool();

    // allocate a node and construct it (placement-new inside)
    CacheNode* createNode(const UnifiedDataPacket& packet);

    // destroy node (explicit destructor) and return slot to pool
    void destroyNode(CacheNode* node);

private:
    struct Impl;
    Impl* pimpl;
};