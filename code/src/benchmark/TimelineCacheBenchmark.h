#include <sys/mman.h>

#include <cstring>

#include <iostream>
#include <map>
#include <deque>
#include <vector>


#include <cstdlib>
#include <cstring>

using std::cerr;
using std::vector;
using std::map;
using std::deque;

// 枚举表示数据类型
enum class DataType {
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

    UnifiedDataPacket() : timestamp(0), type(DataType::CAMERA), data_ptr(nullptr), data_size(0) {} // 添加默认构造函数
    UnifiedDataPacket(long long ts, DataType t, void* ptr, size_t size) : timestamp(ts), type(t), data_ptr(ptr), data_size(size) {} // 添加构造函数

    // 拷贝构造函数 (深拷贝共享内存)
    UnifiedDataPacket(const UnifiedDataPacket& other) : timestamp(other.timestamp), type(other.type), data_size(other.data_size) {
        if (other.data_ptr && other.data_size > 0) {
            data_ptr = mmap(nullptr, other.data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (data_ptr == MAP_FAILED) {
                cerr << "mmap failed in copy constructor";
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
                    cerr << "mmap failed in copy constructor";
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
    map<long long, UnifiedDataPacket> data_map;
    deque<UnifiedDataPacket*> data_deque;

public:
    void insert(long long timestamp, DataType type, void* data_ptr, size_t data_size);

    UnifiedDataPacket* find(long long timestamp);

    vector<UnifiedDataPacket> query_range(long long start_ts, long long end_ts);

    UnifiedDataPacket evict_oldest();
    
    void insertCameraData(long long timestamp, const char* image_data, size_t image_size);

    void insertAudioData(long long timestamp, const char* audio_data, size_t audio_size);

    ~TimelineCacheBaseLine() {}
};