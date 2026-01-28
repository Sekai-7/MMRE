#include "TimelineCache.h"
#include "CacheNodePool.h"

// ------------------ AVL 工具函数 ------------------
int TimelineCache::getHeight(CacheNode* n) { 
    return n ? n->height : 0; 
}

int TimelineCache::getBalanceFactor(CacheNode* n) { 
    return n ? getHeight(n->left) - getHeight(n->right) : 0; 
}

void TimelineCache::updateHeight(CacheNode* n) {
    if (n) n->height = 1 + std::max(getHeight(n->left), getHeight(n->right));
}

// RR旋转
CacheNode* TimelineCache::rotateLeft(CacheNode* x) {
    CacheNode* y = x->right;
    CacheNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    y->parent = x->parent;
    x->parent = y;
    if (T2 != nullptr) {
        T2->parent = x;
    }

    updateHeight(x);
    updateHeight(y);
    return y;
}

// LL旋转
CacheNode* TimelineCache::rotateRight(CacheNode* y) {
    CacheNode* x = y->left;
    CacheNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    x->parent = y->parent;
    y->parent = x;
    if (T2 != nullptr) {
        T2->parent = y;
    }

    updateHeight(y);
    updateHeight(x);
    return x;
}

CacheNode* TimelineCache::rebalance(CacheNode* n) {
    updateHeight(n);
    int bf = getBalanceFactor(n);

    if (bf > 1) {
        if (getBalanceFactor(n->left) < 0)
            n->left = rotateLeft(n->left);
        return rotateRight(n);
    }
    if (bf < -1) {
        if (getBalanceFactor(n->right) > 0)
            n->right = rotateRight(n->right);
        return rotateLeft(n);
    }
    return n;
}

// ------------------ 插入 ------------------
CacheNode* TimelineCache::insert(CacheNode* node, UnifiedDataPacket&& packet, CacheNode*& inserted) {
    // tree is empty or visit lead node
    if (node == nullptr) {
        currentMemoryUsage += packet.dataSize;
        ++currentSize;
        inserted = nodePool->createNode(std::move(packet));
        return inserted;
    }

    Timestamp ts = packet.timestamp;

    // The value of inserted node is less than 
    if (ts < node->timestamp) {
        bool leftIsNull = (node->left == nullptr);
        CacheNode* child = insert(node->left, std::move(packet), inserted);
        node->left = child;
        child->parent = node;

        // 优化：在插入时直接维护链表，避免二次遍历
        if (leftIsNull && inserted) {
            // 当前节点在nodeMap里面不存在，发生错误
            if (nodeMap.find(node) == nodeMap.end()) {
                // 目前没处理
            } else {
                auto ret = list.insert(nodeMap[node], inserted);
                nodeMap[inserted] = ret;
            }
        }
    } else if (ts > node->timestamp) {
        bool rightIsNull = (node->right == nullptr);
        CacheNode* child = insert(node->right, std::move(packet), inserted);
        node->right = child;
        child->parent = node;

        // 优化：在插入时直接维护链表，避免二次遍历
        if (rightIsNull && inserted) {
            if (nodeMap.find(node) == nodeMap.end()) {
                // 目前没处理
            } else {
                auto it = ++nodeMap[node];
                auto ret = list.insert(it, inserted);
                nodeMap[inserted] = ret;
            }
        }
    } else {
        // 覆盖
        // 此时不需要对list做操作
        // Timestamp可能一样吗？
        // 如何对原节点做处理？
        currentMemoryUsage -= (long long)node->packet->dataSize;
        node->packet = std::make_unique<UnifiedDataPacket>(std::move(packet));
        currentMemoryUsage += (long long)node->packet->dataSize;
        inserted = nullptr;
        return node;
    }
    return rebalance(node);
}

void TimelineCache::rebalanceUp(CacheNode* node) {
    while (node) {
        CacheNode* p = node->parent;
        rebalance(node);
        node = p;
    }
}

void TimelineCache::deleteNode(CacheNode* node) {
    // 1. 如果有两个子节点，交换后继节点的数据和链表位置，转化为删除后继节点（0或1个子节点）的情况
    if (node->left != nullptr && node->right != nullptr) {
        auto next = std::next(nodeMap[node]);
        CacheNode* succ = *next; // 在有序链表中，nextByTime 即为后继
        std::swap(node->timestamp, succ->timestamp);
        std::swap(node->packet, succ->packet);

        node = succ; // 现在要删除的目标变成了 succ
    }

    // 2. 处理 0 或 1 个子节点的情况
    list.remove(node);
    nodeMap.erase(node);
    CacheNode* child = node->left != nullptr ? node->left : node->right;
    CacheNode* parent = node->parent;

    if (child != nullptr) {
        child->parent = parent;
    }
    if (parent == nullptr) {
        root = child;
    } else if (parent->left == node) {
        parent->left = child;
    } else {
        parent->right = child;
    }

    // 3. 向上回溯平衡
    rebalanceUp(parent);
    nodePool->destroyNode(node);
}

// ------------------ 辅助函数 ------------------
CacheNode* TimelineCache::findNode(Timestamp ts) const {
    CacheNode* cur = root;
    while (cur) {
        if (ts < cur->timestamp) cur = cur->left;
        else if (ts > cur->timestamp) cur = cur->right;
        else return cur;
    }
    return nullptr;
}

void TimelineCache::refreshMinMaxAfterChange() {
    if (list.empty()) {
        minTimestamp = LLONG_MAX;
        maxTimestamp = LLONG_MIN;
    } else {
        minTimestamp = (*list.begin())->timestamp;
        maxTimestamp = (*list.rbegin())->timestamp;
    }
}

void TimelineCache::queryRange(CacheNode* node, Timestamp l, Timestamp r, std::vector<Handle>& out) const {
    if (!node) return;
    if (node->timestamp > l) queryRange(node->left, l, r, out);
    if (node->timestamp >= l && node->timestamp <= r) out.push_back(node->packet->dataPtr);
    if (node->timestamp < r) queryRange(node->right, l, r, out);
}

void TimelineCache::destroy(CacheNode* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    // delete node;
    nodePool->destroyNode(node);
}

// ------------------ 公共接口 ------------------
TimelineCache::TimelineCache()
    : root(nullptr), list(),
      currentSize(0), currentMemoryUsage(0),
      nodePool(std::make_unique<CacheNodePool>()),
      minTimestamp(LLONG_MAX), maxTimestamp(LLONG_MIN) {}

TimelineCache::~TimelineCache() {
    destroy(root);
}

void TimelineCache::insert(UnifiedDataPacket&& packet) {
    CacheNode* inserted = nullptr;
    long long size = packet.dataSize;

    // 处理空树的特殊情况
    if (root == nullptr) {
        inserted = nodePool->createNode(std::move(packet));
        root = inserted;
        list.push_back(inserted);
        nodeMap.emplace(inserted, list.begin());
        currentSize++;
        currentMemoryUsage += size;
    } else {
        root = insert(root, std::move(packet), inserted);
    }

    refreshMinMaxAfterChange();

    return;
}

bool TimelineCache::remove(Timestamp timestamp) {
    CacheNode* target = findNode(timestamp);
    if (target == nullptr) {
        return false;
    }

    long long removedSize = target->packet->dataSize;
    deleteNode(target);

    currentSize--;
    currentMemoryUsage -= removedSize;

    refreshMinMaxAfterChange();
    return true;
}

Handle TimelineCache::query(Timestamp timestamp) {
    CacheNode* n = findNode(timestamp);
    if (n) return n->packet->dataPtr;
    // return UnifiedDataPacket{};
    return nullptr;
}

std::vector<Handle> TimelineCache::queryByRange(Timestamp startTs, Timestamp endTs) {
    if (startTs > endTs || !root) 
        return {};
    std::vector<Handle> out;
    queryRange(root, startTs, endTs, out);
    return out;
}

void TimelineCache::clear() {
    destroy(root);

    root = nullptr;
    list.clear();
    currentSize = 0;
    currentMemoryUsage = 0;
    minTimestamp = LLONG_MAX;
    maxTimestamp = LLONG_MIN;

    return;
}

Timestamp TimelineCache::getMinTimestamp() const { return minTimestamp; }
Timestamp TimelineCache::getMaxTimestamp() const { return maxTimestamp; }
long long TimelineCache::size() { return currentSize; }
long long TimelineCache::memoryUsage() { return currentMemoryUsage; }