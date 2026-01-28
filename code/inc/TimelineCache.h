#ifndef TIMELINECACHE_H
#define TIMELINECACHE_H

// #include "SharedMemoryManager.h"
#include "common.h"

#include <sys/mman.h>

#include <iostream>
#include <algorithm>
#include <thread>
#include <variant>
#include <list>
#include <unordered_map>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstring>

struct CacheNode;
struct CacheNodePool;

class TimelineCache {
public:
    explicit TimelineCache();
    ~TimelineCache();

    void clear();

    void insert(UnifiedDataPacket&& packet);
    bool remove(Timestamp timestamp);
    Handle query(Timestamp timestamp);
    std::vector<Handle> queryByRange(Timestamp startTs, Timestamp endTs);

    // UnifiedDataPacket evictOldest();
    // UnifiedDataPacket evictNewest();

    Timestamp getMinTimestamp() const;
    Timestamp getMaxTimestamp() const;

    long long size();
    long long memoryUsage();

private:
    CacheNode* root;
    std::list<CacheNode*> list;
    std::unordered_map<CacheNode*, std::list<CacheNode*>::iterator> nodeMap;

    // CacheNode* listHead;
    // CacheNode* listTail;

    long long currentSize;
    long long currentMemoryUsage;

    std::unique_ptr<CacheNodePool> nodePool;

    Timestamp minTimestamp;
    Timestamp maxTimestamp;

private:
    // --- AVL 辅助 ---
    static int getHeight(CacheNode* n);
    static int getBalanceFactor(CacheNode* n);
    static void updateHeight(CacheNode* n);

    CacheNode* rotateLeft(CacheNode* x);
    CacheNode* rotateRight(CacheNode* y);
    CacheNode* rebalance(CacheNode* n);

    CacheNode* insert(CacheNode* node, UnifiedDataPacket&& packet, CacheNode*& inserted);
    void deleteNode(CacheNode* node);
    void rebalanceUp(CacheNode* node);

    void queryRange(CacheNode* node, Timestamp, Timestamp, std::vector<Handle>& out) const;

    void destroy(CacheNode* node);
    CacheNode* findNode(Timestamp ts) const;

    // CacheNode* findPredecessor(Timestamp ts) const;
    // CacheNode* findSuccessor(Timestamp ts) const;

    // void linkIntoList(CacheNode* node);
    // void unlinkFromList(CacheNode* node);
    // void relinkIntoList(CacheNode* node);

    void refreshMinMaxAfterChange();


};

#endif