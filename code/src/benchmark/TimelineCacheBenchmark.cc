#include "TimelineCacheBenchmark.h"

#define LLONG_MAX 10000
#define LLONG_MIN 0
#include <limits>



void TimelineCacheBaseLine::insert(long long timestamp, DataType type, void* dataPtr, size_t dataSize) {
    dataMap[timestamp] = make_shared<UnifiedDataPacketBenchmark>(timestamp, type, dataPtr, dataSize);
    dataDeque.push_back(timestamp);
}

shared_ptr<UnifiedDataPacketBenchmark> TimelineCacheBaseLine::find(long long timestamp) {
    // 使用 map 进行查找 (O(log N))
    auto it = dataMap.find(timestamp);
    if (it != dataMap.end()) {
        return it->second;
    }
    return nullptr;
}

vector<shared_ptr<UnifiedDataPacketBenchmark>> TimelineCacheBaseLine::queryRange(long long startTs, long long endTs) {
    // 利用 map 的有序性进行范围查询 (O(log N + K))
    std::vector<shared_ptr<UnifiedDataPacketBenchmark>> results;
    auto itStart = dataMap.lower_bound(startTs);
    auto itEnd = dataMap.upper_bound(endTs);
    for (auto it = itStart; it != itEnd; ++it) {
        results.push_back(it->second);
    }
    return results;
}

shared_ptr<UnifiedDataPacketBenchmark> TimelineCacheBaseLine::evictOldest() {
    if (dataDeque.empty()) {
        throw std::runtime_error("Cache is empty");
    }
    shared_ptr<UnifiedDataPacketBenchmark> oldestPacket = dataMap[dataDeque.front()];
    dataDeque.pop_front();
    // 2. 从 map 中删除对应的数据 (O(log N))。这里需要注意的是，由于deque中存储的是指针，因此删除map中的元素后，deque中的指针会失效，所以需要先拷贝数据再删除
    dataMap.erase(oldestPacket->timestamp);
    return oldestPacket;
}

void TimelineCacheBaseLine::clear() {
    dataMap.clear();
    dataDeque.clear();
}

void TimelineCacheBaseLine::insertCameraData(long long timestamp, const char* imageData, size_t imageSize) {
    void* buf = malloc(imageSize);
    if (!buf) throw std::bad_alloc();
    memcpy(buf, imageData, imageSize);
    insert(timestamp, DataType::CAMERA, buf, imageSize);
}

void TimelineCacheBaseLine::insertAudioData(long long timestamp, const char* audioData, size_t audioSize) {
    void* buf = malloc(audioSize);
    if (!buf) throw std::bad_alloc();
    memcpy(buf, audioData, audioSize);
    insert(timestamp, DataType::AUDIO, buf, audioSize);
}

CacheNodeBenchmark::CacheNodeBenchmark(long long ts, const UnifiedDataPacketBenchmark& p)
    : timestamp(ts), packet(p),
      parent(nullptr), left(nullptr), right(nullptr),
      height(1), prevByTime(nullptr), nextByTime(nullptr) {}

// ------------------ AVL 工具函数 ------------------
int TimelineCacheBenchmark::getHeight(CacheNodeBenchmark* n) { return n ? n->height : 0; }
int TimelineCacheBenchmark::getBalanceFactor(CacheNodeBenchmark* n) { return n ? getHeight(n->left) - getHeight(n->right) : 0; }
void TimelineCacheBenchmark::updateHeight(CacheNodeBenchmark* n) {
    if (n) n->height = 1 + std::max(getHeight(n->left), getHeight(n->right));
}

CacheNodeBenchmark* TimelineCacheBenchmark::rotateLeft(CacheNodeBenchmark* x) {
    CacheNodeBenchmark* y = x->right;
    CacheNodeBenchmark* T2 = y->left;

    y->left = x;
    x->right = T2;

    y->parent = x->parent;
    x->parent = y;
    if (T2) T2->parent = x;

    if (!y->parent) root = y;
    else if (y->parent->left == x) y->parent->left = y;
    else y->parent->right = y;

    updateHeight(x);
    updateHeight(y);
    return y;
}

CacheNodeBenchmark* TimelineCacheBenchmark::rotateRight(CacheNodeBenchmark* y) {
    CacheNodeBenchmark* x = y->left;
    CacheNodeBenchmark* T2 = x->right;

    x->right = y;
    y->left = T2;

    x->parent = y->parent;
    y->parent = x;
    if (T2) T2->parent = y;

    if (!x->parent) root = x;
    else if (x->parent->left == y) x->parent->left = x;
    else x->parent->right = x;

    updateHeight(y);
    updateHeight(x);
    return x;
}

CacheNodeBenchmark* TimelineCacheBenchmark::rebalance(CacheNodeBenchmark* n) {
    updateHeight(n);
    int bf = getBalanceFactor(n);

    if (bf > 1) {
        if (getBalanceFactor(n->left) < 0)
            rotateLeft(n->left);
        return rotateRight(n);
    }
    if (bf < -1) {
        if (getBalanceFactor(n->right) > 0)
            rotateRight(n->right);
        return rotateLeft(n);
    }
    return n;
}

// ------------------ 插入 ------------------
CacheNodeBenchmark* TimelineCacheBenchmark::insert(CacheNodeBenchmark* node, long long ts, const UnifiedDataPacketBenchmark& packet, CacheNodeBenchmark*& inserted) {
    //if (!node) {
    //    inserted = new CacheNodeBenchmark(ts, packet);
    //    return inserted;
    //}

    if (!node) {
        inserted = nodePool.createNode(ts, packet); // 使用池分配并构造
        return inserted;
    }
    if (ts < node->timestamp) {
        CacheNodeBenchmark* child = insert(node->left, ts, packet, inserted);
        node->left = child;
        child->parent = node;
    } else if (ts > node->timestamp) {
        CacheNodeBenchmark* child = insert(node->right, ts, packet, inserted);
        node->right = child;
        child->parent = node;
    } else {
        // 覆盖
        currentMemoryUsage -= (long long)node->packet.dataSize;
        node->packet = packet;
        currentMemoryUsage += (long long)node->packet.dataSize;
        inserted = node;
        return node;
    }
    return rebalance(node);
}

// ------------------ 删除（纯 AVL，不处理统计/链表） ------------------
CacheNodeBenchmark* TimelineCacheBenchmark::removePureAVL(CacheNodeBenchmark* node, long long ts, CacheNodeBenchmark*& deletedNode) {
    if (!node) return nullptr;

    if (ts < node->timestamp) {
        node->left = removePureAVL(node->left, ts, deletedNode);
        if (node->left) node->left->parent = node;
    } else if (ts > node->timestamp) {
        node->right = removePureAVL(node->right, ts, deletedNode);
        if (node->right) node->right->parent = node;
    } else {
        if (!node->left || !node->right) {
            CacheNodeBenchmark* child = node->left ? node->left : node->right;
            if (child) child->parent = node->parent;
            deletedNode = node;
            return child;
        } else {
            CacheNodeBenchmark* succ = node->right;
            while (succ->left) succ = succ->left;
            std::swap(node->timestamp, succ->timestamp);
            std::swap(node->packet, succ->packet);
            node->right = removePureAVL(node->right, ts, deletedNode);
            if (node->right) node->right->parent = node;
        }
    }
    return rebalance(node);
}

// ------------------ 辅助函数 ------------------
CacheNodeBenchmark* TimelineCacheBenchmark::findNode(long long ts) const {
    CacheNodeBenchmark* cur = root;
    while (cur) {
        if (ts < cur->timestamp) cur = cur->left;
        else if (ts > cur->timestamp) cur = cur->right;
        else return cur;
    }
    return nullptr;
}

CacheNodeBenchmark* TimelineCacheBenchmark::findPredecessor(long long ts) const {
    CacheNodeBenchmark* cur = root;
    CacheNodeBenchmark* pred = nullptr;
    while (cur) {
        if (ts > cur->timestamp) {
            pred = cur;
            cur = cur->right;
        } else cur = cur->left;
    }
    return pred;
}

CacheNodeBenchmark* TimelineCacheBenchmark::findSuccessor(long long ts) const {
    CacheNodeBenchmark* cur = root;
    CacheNodeBenchmark* succ = nullptr;
    while (cur) {
        if (ts < cur->timestamp) {
            succ = cur;
            cur = cur->left;
        } else cur = cur->right;
    }
    return succ;
}

void TimelineCacheBenchmark::linkIntoList(CacheNodeBenchmark* node) {
    if (!listHead) {
        listHead = listTail = node;
        return;
    }
    CacheNodeBenchmark* pred = findPredecessor(node->timestamp);
    if (pred) {
        node->prevByTime = pred;
        node->nextByTime = pred->nextByTime;
        pred->nextByTime = node;
    } else {
        node->nextByTime = listHead;
        listHead->prevByTime = node;
        listHead = node;
    }
    if (node->nextByTime) node->nextByTime->prevByTime = node;
    else listTail = node;
}

void TimelineCacheBenchmark::unlinkFromList(CacheNodeBenchmark* node) {
    if (!node) return;
    if (node->prevByTime) node->prevByTime->nextByTime = node->nextByTime;
    else listHead = node->nextByTime;
    if (node->nextByTime) node->nextByTime->prevByTime = node->prevByTime;
    else listTail = node->prevByTime;
    node->prevByTime = node->nextByTime = nullptr;
}

void TimelineCacheBenchmark::relinkIntoList(CacheNodeBenchmark* node) {
    unlinkFromList(node);
    linkIntoList(node);
}

void TimelineCacheBenchmark::refreshMinMaxAfterChange() {
    if (!listHead) {
        minTimestamp = LLONG_MAX;
        maxTimestamp = LLONG_MIN;
    } else {
        minTimestamp = listHead->timestamp;
        maxTimestamp = listTail->timestamp;
    }
}
//void TimelineCacheBenchmark::refreshMinMaxAfterChange() {
//    if (!listHead) {
//        minTimestamp = std::numeric_limits<long long>::max();
//        maxTimestamp = std::numeric_limits<long long>::min();
//    } else {
//        minTimestamp = listHead->timestamp;
//        maxTimestamp = listTail->timestamp;
//    }
//}


void TimelineCacheBenchmark::queryRange(CacheNodeBenchmark* node, long long l, long long r, std::vector<UnifiedDataPacketBenchmark>& out) const {
    if (!node) return;
    if (node->timestamp > l) queryRange(node->left, l, r, out);
    if (node->timestamp >= l && node->timestamp <= r) out.push_back(node->packet);
    if (node->timestamp < r) queryRange(node->right, l, r, out);
}

void TimelineCacheBenchmark::destroy(CacheNodeBenchmark* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    //delete node;
    // 不要直接 delete，因为节点来自池
    nodePool.destroyNode(node);

}

// ------------------ 公共接口 ------------------
TimelineCacheBenchmark::TimelineCacheBenchmark(CacheNodePoolBenchmark& pool)
    : root(nullptr), listHead(nullptr), listTail(nullptr),
      currentSize(0), currentMemoryUsage(0),
      nodePool(pool),
      minTimestamp(LLONG_MAX), maxTimestamp(LLONG_MIN) {}

TimelineCacheBenchmark::~TimelineCacheBenchmark() {
    destroy(root);
}

void TimelineCacheBenchmark::insert(long long timestamp, UnifiedDataPacketBenchmark packet) {
    packet.timestamp = timestamp;
    CacheNodeBenchmark* inserted = nullptr;
    root = insert(root, timestamp, packet, inserted);
    if (inserted && inserted->prevByTime == nullptr && inserted->nextByTime == nullptr) {
        linkIntoList(inserted);
        currentSize++;
        currentMemoryUsage += (long long)packet.dataSize;
    }
    refreshMinMaxAfterChange();
}

bool TimelineCacheBenchmark::remove(long long timestamp) {
    CacheNodeBenchmark* target = findNode(timestamp);
    if (!target) return false;
    unlinkFromList(target);
    currentSize--;
    currentMemoryUsage -= (long long)target->packet.dataSize;
    CacheNodeBenchmark* deletedNode = nullptr;
    root = removePureAVL(root, timestamp, deletedNode);
    //if (deletedNode) delete deletedNode;
    if (deletedNode) 
    {
        nodePool.destroyNode(deletedNode);
        deletedNode = nullptr;
    }

    refreshMinMaxAfterChange();
    return true;
}

UnifiedDataPacketBenchmark TimelineCacheBenchmark::find(long long timestamp) {
    CacheNodeBenchmark* n = findNode(timestamp);
    if (n) return n->packet;
    return UnifiedDataPacketBenchmark{};
}

std::vector<UnifiedDataPacketBenchmark> TimelineCacheBenchmark::queryRange(long long startTs, long long endTs) {
    std::vector<UnifiedDataPacketBenchmark> out;
    if (!root || startTs > endTs) return out;
    queryRange(root, startTs, endTs, out);
    return out;
}

UnifiedDataPacketBenchmark TimelineCacheBenchmark::evictOldest() {
    if (!listHead) throw std::runtime_error("Cache empty");
    long long ts = listHead->timestamp;
    UnifiedDataPacketBenchmark ret = listHead->packet;
    remove(ts);
    return ret;
}

UnifiedDataPacketBenchmark TimelineCacheBenchmark::evictNewest() {
    if (!listTail) throw std::runtime_error("Cache empty");
    long long ts = listTail->timestamp;
    UnifiedDataPacketBenchmark ret = listTail->packet;
    remove(ts);
    return ret;
}

void TimelineCacheBenchmark::clear() {
    destroy(root);
    root = nullptr;
    listHead = listTail = nullptr;
    currentSize = 0;
    currentMemoryUsage = 0;
    refreshMinMaxAfterChange();
}

long long TimelineCacheBenchmark::getMinTimestamp() const { return minTimestamp; }
long long TimelineCacheBenchmark::getMaxTimestamp() const { return maxTimestamp; }
long long TimelineCacheBenchmark::size() { return currentSize; }
long long TimelineCacheBenchmark::memoryUsage() { return currentMemoryUsage; }