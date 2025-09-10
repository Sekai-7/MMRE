#include "TimelineCacheBenchmark.h"

#define LLONG_MAX 10000
#define LLONG_MIN 0
#include <limits>



void TimelineCacheBaseLine::insert(long long timestamp, DataType type, void* data_ptr, size_t data_size) {
    data_map[timestamp] = make_shared<UnifiedDataPacket>(timestamp, type, data_ptr, data_size);
    data_deque.push_back(timestamp);
}

shared_ptr<UnifiedDataPacket> TimelineCacheBaseLine::find(long long timestamp) {
    // 使用 map 进行查找 (O(log N))
    auto it = data_map.find(timestamp);
    if (it != data_map.end()) {
        return it->second;
    }
    return nullptr;
}

vector<shared_ptr<UnifiedDataPacket>> TimelineCacheBaseLine::query_range(long long start_ts, long long end_ts) {
    // 利用 map 的有序性进行范围查询 (O(log N + K))
    std::vector<shared_ptr<UnifiedDataPacket>> results;
    auto it_start = data_map.lower_bound(start_ts);
    auto it_end = data_map.upper_bound(end_ts);
    for (auto it = it_start; it != it_end; ++it) {
        results.push_back(it->second);
    }
    return results;
}

shared_ptr<UnifiedDataPacket> TimelineCacheBaseLine::evict_oldest() {
    if (data_deque.empty()) {
        throw std::runtime_error("Cache is empty");
    }
    shared_ptr<UnifiedDataPacket> oldest_packet = data_map[data_deque.front()];
    data_deque.pop_front();
    // 2. 从 map 中删除对应的数据 (O(log N))。这里需要注意的是，由于deque中存储的是指针，因此删除map中的元素后，deque中的指针会失效，所以需要先拷贝数据再删除
    data_map.erase(oldest_packet->timestamp);
    return oldest_packet;
}
// void TimelineCacheBaseLine::insertCameraData(long long timestamp, const char* image_data, size_t image_size) {
//     void* shm_ptr = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//     if (shm_ptr == MAP_FAILED) {
//         perror("mmap failed");
//         // ... error handling
//     }
//     memcpy(shm_ptr, image_data, image_size);
//     insert(timestamp, DataType::CAMERA, shm_ptr, image_size);
// }

// void TimelineCacheBaseLine::insertAudioData(long long timestamp, const char* audio_data, size_t audio_size) {
//     void* shm_ptr = mmap(NULL, audio_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//     if (shm_ptr == MAP_FAILED) {
//         perror("mmap failed");
//         // ... error handling
//     }
//     memcpy(shm_ptr, audio_data, audio_size);
//     insert(timestamp, DataType::AUDIO, shm_ptr, audio_size);
// }

void TimelineCacheBaseLine::insertCameraData(long long timestamp, const char* image_data, size_t image_size) {
    void* buf = malloc(image_size);
    if (!buf) throw std::bad_alloc();
    memcpy(buf, image_data, image_size);
    insert(timestamp, DataType::CAMERA, buf, image_size);
}

void TimelineCacheBaseLine::insertAudioData(long long timestamp, const char* audio_data, size_t audio_size) {
    void* buf = malloc(audio_size);
    if (!buf) throw std::bad_alloc();
    memcpy(buf, audio_data, audio_size);
    insert(timestamp, DataType::AUDIO, buf, audio_size);
}

CacheNode::CacheNode(long long ts, const UnifiedDataPacket& p)
    : timestamp(ts), packet(p),
      parent(nullptr), left(nullptr), right(nullptr),
      height(1), prev_by_time(nullptr), next_by_time(nullptr) {}

// ------------------ AVL 工具函数 ------------------
int TimelineCache::get_height(CacheNode* n) { return n ? n->height : 0; }
int TimelineCache::get_balance_factor(CacheNode* n) { return n ? get_height(n->left) - get_height(n->right) : 0; }
void TimelineCache::update_height(CacheNode* n) {
    if (n) n->height = 1 + std::max(get_height(n->left), get_height(n->right));
}

CacheNode* TimelineCache::rotate_left(CacheNode* x) {
    CacheNode* y = x->right;
    CacheNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    y->parent = x->parent;
    x->parent = y;
    if (T2) T2->parent = x;

    if (!y->parent) root = y;
    else if (y->parent->left == x) y->parent->left = y;
    else y->parent->right = y;

    update_height(x);
    update_height(y);
    return y;
}

CacheNode* TimelineCache::rotate_right(CacheNode* y) {
    CacheNode* x = y->left;
    CacheNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    x->parent = y->parent;
    y->parent = x;
    if (T2) T2->parent = y;

    if (!x->parent) root = x;
    else if (x->parent->left == y) x->parent->left = x;
    else x->parent->right = x;

    update_height(y);
    update_height(x);
    return x;
}

CacheNode* TimelineCache::rebalance(CacheNode* n) {
    update_height(n);
    int bf = get_balance_factor(n);

    if (bf > 1) {
        if (get_balance_factor(n->left) < 0)
            rotate_left(n->left);
        return rotate_right(n);
    }
    if (bf < -1) {
        if (get_balance_factor(n->right) > 0)
            rotate_right(n->right);
        return rotate_left(n);
    }
    return n;
}

// ------------------ 插入 ------------------
CacheNode* TimelineCache::insert(CacheNode* node, long long ts, const UnifiedDataPacket& packet, CacheNode*& inserted) {
    //if (!node) {
    //    inserted = new CacheNode(ts, packet);
    //    return inserted;
    //}

    if (!node) {
        inserted = node_pool.createNode(ts, packet); // 使用池分配并构造
        return inserted;
    }
    if (ts < node->timestamp) {
        CacheNode* child = insert(node->left, ts, packet, inserted);
        node->left = child;
        child->parent = node;
    } else if (ts > node->timestamp) {
        CacheNode* child = insert(node->right, ts, packet, inserted);
        node->right = child;
        child->parent = node;
    } else {
        // 覆盖
        current_memory_usage -= (long long)node->packet.data_size;
        node->packet = packet;
        current_memory_usage += (long long)node->packet.data_size;
        inserted = node;
        return node;
    }
    return rebalance(node);
}

// ------------------ 删除（纯 AVL，不处理统计/链表） ------------------
CacheNode* TimelineCache::remove_pureAVL(CacheNode* node, long long ts, CacheNode*& deleted_node) {
    if (!node) return nullptr;

    if (ts < node->timestamp) {
        node->left = remove_pureAVL(node->left, ts, deleted_node);
        if (node->left) node->left->parent = node;
    } else if (ts > node->timestamp) {
        node->right = remove_pureAVL(node->right, ts, deleted_node);
        if (node->right) node->right->parent = node;
    } else {
        if (!node->left || !node->right) {
            CacheNode* child = node->left ? node->left : node->right;
            if (child) child->parent = node->parent;
            deleted_node = node;
            return child;
        } else {
            CacheNode* succ = node->right;
            while (succ->left) succ = succ->left;
            std::swap(node->timestamp, succ->timestamp);
            std::swap(node->packet, succ->packet);
            node->right = remove_pureAVL(node->right, ts, deleted_node);
            if (node->right) node->right->parent = node;
        }
    }
    return rebalance(node);
}

// ------------------ 辅助函数 ------------------
CacheNode* TimelineCache::find_node(long long ts) const {
    CacheNode* cur = root;
    while (cur) {
        if (ts < cur->timestamp) cur = cur->left;
        else if (ts > cur->timestamp) cur = cur->right;
        else return cur;
    }
    return nullptr;
}

CacheNode* TimelineCache::find_predecessor(long long ts) const {
    CacheNode* cur = root;
    CacheNode* pred = nullptr;
    while (cur) {
        if (ts > cur->timestamp) {
            pred = cur;
            cur = cur->right;
        } else cur = cur->left;
    }
    return pred;
}

CacheNode* TimelineCache::find_successor(long long ts) const {
    CacheNode* cur = root;
    CacheNode* succ = nullptr;
    while (cur) {
        if (ts < cur->timestamp) {
            succ = cur;
            cur = cur->left;
        } else cur = cur->right;
    }
    return succ;
}

void TimelineCache::link_into_list(CacheNode* node) {
    if (!list_head) {
        list_head = list_tail = node;
        return;
    }
    CacheNode* pred = find_predecessor(node->timestamp);
    if (pred) {
        node->prev_by_time = pred;
        node->next_by_time = pred->next_by_time;
        pred->next_by_time = node;
    } else {
        node->next_by_time = list_head;
        list_head->prev_by_time = node;
        list_head = node;
    }
    if (node->next_by_time) node->next_by_time->prev_by_time = node;
    else list_tail = node;
}

void TimelineCache::unlink_from_list(CacheNode* node) {
    if (!node) return;
    if (node->prev_by_time) node->prev_by_time->next_by_time = node->next_by_time;
    else list_head = node->next_by_time;
    if (node->next_by_time) node->next_by_time->prev_by_time = node->prev_by_time;
    else list_tail = node->prev_by_time;
    node->prev_by_time = node->next_by_time = nullptr;
}

void TimelineCache::relink_into_list(CacheNode* node) {
    unlink_from_list(node);
    link_into_list(node);
}

void TimelineCache::refresh_min_max_after_change() {
    if (!list_head) {
        min_timestamp = LLONG_MAX;
        max_timestamp = LLONG_MIN;
    } else {
        min_timestamp = list_head->timestamp;
        max_timestamp = list_tail->timestamp;
    }
}
//void TimelineCache::refresh_min_max_after_change() {
//    if (!list_head) {
//        min_timestamp = std::numeric_limits<long long>::max();
//        max_timestamp = std::numeric_limits<long long>::min();
//    } else {
//        min_timestamp = list_head->timestamp;
//        max_timestamp = list_tail->timestamp;
//    }
//}


void TimelineCache::query_range(CacheNode* node, long long l, long long r, std::vector<UnifiedDataPacket>& out) const {
    if (!node) return;
    if (node->timestamp > l) query_range(node->left, l, r, out);
    if (node->timestamp >= l && node->timestamp <= r) out.push_back(node->packet);
    if (node->timestamp < r) query_range(node->right, l, r, out);
}

void TimelineCache::destroy(CacheNode* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    //delete node;
    // 不要直接 delete，因为节点来自池
    node_pool.destroyNode(node);

}

// ------------------ 公共接口 ------------------
TimelineCache::TimelineCache(CacheNodePool& pool)
    : root(nullptr), list_head(nullptr), list_tail(nullptr),
      current_size(0), current_memory_usage(0),
      node_pool(pool),
      min_timestamp(LLONG_MAX), max_timestamp(LLONG_MIN) {}

TimelineCache::~TimelineCache() {
    destroy(root);
}

void TimelineCache::insert(long long timestamp, UnifiedDataPacket packet) {
    packet.timestamp = timestamp;
    CacheNode* inserted = nullptr;
    root = insert(root, timestamp, packet, inserted);
    if (inserted && inserted->prev_by_time == nullptr && inserted->next_by_time == nullptr) {
        link_into_list(inserted);
        current_size++;
        current_memory_usage += (long long)packet.data_size;
    }
    refresh_min_max_after_change();
}

bool TimelineCache::remove(long long timestamp) {
    CacheNode* target = find_node(timestamp);
    if (!target) return false;
    unlink_from_list(target);
    current_size--;
    current_memory_usage -= (long long)target->packet.data_size;
    CacheNode* deleted_node = nullptr;
    root = remove_pureAVL(root, timestamp, deleted_node);
    //if (deleted_node) delete deleted_node;
    if (deleted_node) 
    {
        node_pool.destroyNode(deleted_node);
        deleted_node = nullptr;
    }

    refresh_min_max_after_change();
    return true;
}

UnifiedDataPacket TimelineCache::find(long long timestamp) {
    CacheNode* n = find_node(timestamp);
    if (n) return n->packet;
    return UnifiedDataPacket{};
}

std::vector<UnifiedDataPacket> TimelineCache::query_range(long long start_ts, long long end_ts) {
    std::vector<UnifiedDataPacket> out;
    if (!root || start_ts > end_ts) return out;
    query_range(root, start_ts, end_ts, out);
    return out;
}

UnifiedDataPacket TimelineCache::evict_oldest() {
    if (!list_head) throw std::runtime_error("Cache empty");
    long long ts = list_head->timestamp;
    UnifiedDataPacket ret = list_head->packet;
    remove(ts);
    return ret;
}

UnifiedDataPacket TimelineCache::evict_newest() {
    if (!list_tail) throw std::runtime_error("Cache empty");
    long long ts = list_tail->timestamp;
    UnifiedDataPacket ret = list_tail->packet;
    remove(ts);
    return ret;
}

long long TimelineCache::get_min_timestamp() const { return min_timestamp; }
long long TimelineCache::get_max_timestamp() const { return max_timestamp; }
long long TimelineCache::size() { return current_size; }
long long TimelineCache::memory_usage() { return current_memory_usage; }