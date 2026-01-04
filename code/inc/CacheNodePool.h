#ifndef CACHENODEPOOL_H
#define CACHENODEPOOL_H

#include "common.h"

#include <atomic>

class CacheNode {
public:
    long long timestamp;
    UnifiedDataPacket packet;

    // AVL 指针
    CacheNode* parent;
    CacheNode* left;
    CacheNode* right;
    int height;

    // 链表指针
    // CacheNode* prevByTime;
    // CacheNode* nextByTime;

    // COW (当前未使用)
    std::atomic<int> refCount{1};
    bool isShared{false};

    explicit CacheNode(UnifiedDataPacket&& p);
};

class CacheNodePool {
public:
    // ctor/dtor
    explicit CacheNodePool(size_t nodesPerSlab = 4096);
    ~CacheNodePool();

    // allocate a node and construct it (placement-new inside)
    CacheNode* createNode(UnifiedDataPacket&& packet);

    // destroy node (explicit destructor) and return slot to pool
    void destroyNode(CacheNode* node);

private:
    struct Impl;
    Impl* impl;
};

#endif