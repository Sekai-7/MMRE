# 子杰伪代码

## 数据精简模块 (Data Reduction Module) 设计
数据精简模块负责缓存数据的生命周期管理和策略驱动的数据持久化，主要功能包括：
- 监控缓存大小并执行淘汰策略
- 管理共享内存引用计数
- 实现缓存到持久存储的同步
- 处理检查点驱动的数据精简
- 支持可配置的持久化策略
## ConfigurationManager
很多支持扩展性的地方都是通过读配置文件实现的，比如有哪些类型的输入数据、输入数据的具体格式等等，这些配置文件都由ConfigurationManager管理。多个需要配置文件的类都会依赖它。

## PersistenceEngine
PersistenceEngine负责处理data sync相关逻辑。它持有一个PersistencePolicyManager，这个Manager会根据具体的数据精简策略（比如抽帧）得到准备存入UFS的数据。这个地方要联动PersistencePolicyManager，它要从ConfigurationManager获取每类数据如何精简化，并执行精简化逻辑，最简单的就是对camera数据抽帧、audio数据抽帧、保存checkpoint等等。这个策略是直接配置好的，可以把原始数据交给一个策略模式去process之后，把精简化好的数据交给ufsadapter。它同时持有一个UFSAdapter，负责真的从UFS读取/往UFS写入数据。

## UFSAdapter
存储抽象层，负责和UFS真正的交互。

## PersistencePolicyManager
数据在取出cache、准备持久化之前，要经过一个精简的流程。但每种数据怎么精简都需要自己的策略，这个类负责管理这些具体的策略。

## DataStoreStrategyFactory
真正精简逻辑策略的工厂模式设计。

## DataStoreStrategy CameraStrategy AudioStrategy
每个具体的执行数据精简逻辑的类，具体的类继承父类DataStoreStrategy,实现processtore函数。

## PersistenceEngine
| 描述                                | 接口                                                                                                                        |
|-----------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| 设置被监控的缓存实例                        | `void set_monitored_cache(MultiResourceTimelineCache* cache);`                                                              |
| 立即触发指定资源类型的数据持久化                | `void trigger_immediate_persistence(MultiResourceTimelineCache* cache, ResourceType type);`                                 |
| 查询持久化存储中的数据                       | `QueryResult query_persisted_data(ResourceType type, Timestamp start_ts, Timestamp end_ts);`                               |
| 开始后台监控线程                          | `void start_monitoring();`                                                                                                 |
| 停止后台监控线程                          | `void stop_monitoring();`                                                                                                  |

## UFSAdapter
| 描述               | 接口                                                                                                                        |
|------------------|---------------------------------------------------------------------------------------------------------------------------|
| 构造函数：默认构造      | `UfsAdapter() = default;`                                                                                                 |
| 析构函数：默认析构      | `~UfsAdapter() = default;`                                                                                                 |
| 持久化数据              | `bool persist_data(ResourceType type, const QueryResult& query_result);`                                                 |
| 写入普通数据            | `bool write_plain_data(long long timestamp, ResourceType type, const std::vector<uint8_t>& data);`                        |
| 写入压缩数据            | `bool write_compressed_data(long long timestamp, ResourceType type, const std::vector<uint8_t>& compressed_data);`       |
| 读取指定时间范围的数据      | `std::vector<PersistedDataInfo> read_data_in_range(const TimeRange& range);`                                              |
| 读取普通存储数据          | `std::vector<uint8_t> read_plain_data(const PersistedDataInfo& info);`                                                    |
| 读取压缩存储数据          | `std::vector<uint8_t> read_compressed_data(const PersistedDataInfo& info);`                                               |


## PersistencePolicyManager
| 描述                                  | 接口                                                                                                                      |
|-------------------------------------|-------------------------------------------------------------------------------------------------------------------------|
| 构造函数：初始化配置管理器并创建策略                | `explicit PersistencePolicyManager(ConfigurationManager* cfg_mgr);`                                                      |
| 根据当前缓存使用情况计算驱逐决策列表                 | `std::vector<EvictionDecision> calculate_eviction_decisions(const std::map<ResourceType, CacheUsageInfo>& usage_info);` |
| 对原始查询结果进行策略处理并返回处理后的数据            | `ProcessedDataResult process_raw_data(ResourceType type, const QueryResult& raw_data);`                                 |


## DataStoreStrategyFactory
| 描述                             | 接口                                                                                                                            |
|--------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| 为指定资源类型创建对应的数据处理策略 | `static std::unique_ptr<DataProcessingStrategy> create_strategy(ResourceType type, const ProcessingConfig& config);`          |


## DataStoreStrategy CameraStrategy AudioStrategy
| 描述                                    | 接口                                                                                                                |
|---------------------------------------|-------------------------------------------------------------------------------------------------------------------|
| 抽象策略基类析构函数                         | `virtual ~DataStoreStrategy() = default;`                                                                          |
| 抽象策略基类：处理原始数据                    | `virtual ProcessedDataResult processtore(const QueryResult& raw_data) = 0;`                                        |
| 抽象策略基类：获取策略名称                    | `virtual std::string get_strategy_name() const = 0;`                                                               |
| 构造函数：Camera 帧抽帧策略                    | `explicit CameraFrameSubsamplingStrategy(int ratio);`                                                              |
| Camera 帧抽帧：处理原始数据                    | `ProcessedDataResult processtore(const QueryResult& raw_data) override;`                                           |
| Camera 帧抽帧：获取策略名称                    | `std::string get_strategy_name() const override;`                                                                  |
| 构造函数：Audio 降采样策略                    | `explicit AudioDownsamplingStrategy(int factor);`                                                                  |
| Audio 降采样：处理原始数据                    | `ProcessedDataResult processtore(const QueryResult& raw_data) override;`                                           |
| Audio 降采样：获取策略名称                    | `std::string get_strategy_name() const override;`                                                                  |
| 构造函数：检查点压缩策略                      | `explicit CheckpointCompressionStrategy(std::chrono::milliseconds interval);`                                       |
| 检查点压缩：处理原始数据                      | `ProcessedDataResult process(const QueryResult& raw_data) override;`                                               |
| 检查点压缩：获取策略名称                      | `std::string get_strategy_name() const override;`                                                                  |
| 工厂方法：创建指定资源类型的数据处理策略              | `static std::unique_ptr<DataProcessingStrategy> create_strategy(ResourceType type, const ProcessingConfig& config);` |




### 2 持久化策略管理器 PersistencePolicyManager
``` c++
class PersistencePolicyManager {
private:
    ConfigurationManager* config_manager;
    std::map<ResourceType, std::unique_ptr<DataProcessingStrategy>> processing_strategies;

public:
    explicit PersistencePolicyManager(ConfigurationManager* cfg_mgr)
        : config_manager(cfg_mgr) {
        initialize_processing_strategies();
    }

    std::vector<EvictionDecision> calculate_eviction_decisions(
        const std::map<ResourceType, CacheUsageInfo>& usage_info) {
        
        std::vector<EvictionDecision> decisions;
        
        for (const auto& [type, info] : usage_info) {
            size_t size_limit = config_manager->get_cache_size_limit(type);
            
            if (info.current_size > size_limit) {
                Timestamp evict_before = calculate_eviction_timestamp(type, info, size_limit);
                decisions.emplace_back(EvictionDecision{
                    .resource_type = type,
                    .evict_before_timestamp = evict_before,
                    .reason = EvictionReason::SIZE_LIMIT_EXCEEDED
                });
            }
        }
        
        return decisions;
    }

    ProcessedDataResult process_raw_data(ResourceType type, const QueryResult& raw_data) {
        auto it = processing_strategies.find(type);
        if (it == processing_strategies.end()) {
            throw std::runtime_error("No processing strategy found for resource type");
        }
        
        return it->second->process(raw_data);
    }

private:
    void initialize_processing_strategies() {
        auto processing_config = config_manager->get_processing_config();
        
        for (auto type : {ResourceType::CAMERA, ResourceType::AUDIO, 
                          ResourceType::VEHICLE_SIGNAL, ResourceType::GPS, ResourceType::IMU}) {
            processing_strategies[type] = DataStoreStrategyFactory::create_strategy(type, processing_config);
        }
    }
    
    Timestamp calculate_eviction_timestamp(ResourceType type, const CacheUsageInfo& info, size_t size_limit) {
        auto retention_policy = config_manager->get_retention_policy(type);
        return info.oldest_timestamp + retention_policy.retention_duration;
    }
};
```
### 3 持久化处理器  PersistenceEngine
``` c++
class PersistenceEngine {
private:
    std::unique_ptr<PersistencePolicyManager> policy_manager;
    std::unique_ptr<ConfigurationManager> config_manager;
    std::unique_ptr<UfsAdapter> ufs_adapter;
    MultiResourceTimelineCache* monitored_cache = nullptr;
    
    // 监控线程
    std::thread monitoring_thread;
    std::atomic<bool> should_stop{false};
    std::condition_variable cv;
    std::mutex cv_mutex;

public:
    //cache把自己传给它 方便监测
    void set_monitored_cache(MultiResourceTimelineCache* cache) {
        monitored_cache = cache;
    }

    void trigger_immediate_persistence(MultiResourceTimelineCache* cache, ResourceType type) {
        if (!cache) return;
        auto usage_info = cache->get_cache_usage_info(type);
        std::map<ResourceType, CacheUsageInfo> single_usage_map{{type, usage_info}};
        auto decisions = policy_manager->calculate_eviction_decisions(single_usage_map);
        execute_persistence_decisions(cache, decisions);
    }

    QueryResult query_persisted_data(ResourceType type, Timestamp start_ts, Timestamp end_ts) {
        try {
            // 从UFS查询持久化的数据
            PersistenceQuery query;
            query.resource_type = type;
            query.start_timestamp = start_ts;
            query.end_timestamp = end_ts;
            
            auto persisted_bundle = ufs_adapter->query_persisted_data(query);
            
            if (persisted_bundle.has_value()) {
                return QueryResult::create_success(type, persisted_bundle->processed_data.packets);
            } else {
                return QueryResult::create_empty(type);
            }
            
        } catch (const std::exception& e) {
            log_error("Failed to query persisted data: " + std::string(e.what()));
            return QueryResult::create_error(type, "Persistence query failed");
        }
    }

    void start_monitoring() {
        if (!monitoring_thread.joinable()) {
            monitoring_thread = std::thread(&PersistenceEngine::monitoring_loop, this);
        }
    }

    void stop_monitoring() {
        should_stop = true;
        cv.notify_all();
        if (monitoring_thread.joinable()) {
            monitoring_thread.join();
        }
    }

private:
    void monitoring_loop() {
        while (!should_stop) {
            try {
                if (!monitored_cache) {
                    // 如果没有设置监控目标，等待后继续
                    std::unique_lock<std::mutex> lock(cv_mutex);
                    cv.wait_for(lock, std::chrono::seconds(1));
                    continue;
                }
                
                // 获取所有缓存的使用情况
                auto usage_info = monitored_cache->get_all_cache_usage_info();
                
                // 让策略管理器决定需要持久化的数据 决策以decisions数据结构的形式返回
                auto decisions = policy_manager->calculate_eviction_decisions(usage_info);
                
                if (!decisions.empty()) {
                    execute_persistence_decisions(monitored_cache, decisions);
                }
                
                // 等待下次检查
                std::unique_lock<std::mutex> lock(cv_mutex);
                cv.wait_for(lock, std::chrono::seconds(config_manager->get_monitoring_interval()));
                
            } catch (const std::exception& e) {
                log_error("Error in monitoring loop: " + std::string(e.what()));
                continue;
            }
        }
    }

    void execute_persistence_decisions(MultiResourceTimelineCache* cache, 
                                     const std::vector<EvictionDecision>& decisions) {
        if (!cache) return;
        
        for (const auto& decision : decisions) {
            try {
                // 1. 查询相关的检查点信息
                auto relevant_checkpoints = cache->query_checkpoints_before(
                    decision.resource_type, 
                    decision.evict_before_timestamp
                );
                
                // 2. 查询需要持久化的原始数据
                auto raw_data = cache->query_by_range(
                    decision.resource_type, 
                    0,
                    decision.evict_before_timestamp,
                    false
                );
                
                if (raw_data.packets.empty() && relevant_checkpoints.empty()) {
                    continue;
                }
                
                // 3-6. 处理和持久化
                ProcessingContext context;
                context.checkpoints = relevant_checkpoints;
                context.eviction_timestamp = decision.evict_before_timestamp;
                
                auto processed_data = policy_manager->process_raw_data_with_context(
                    decision.resource_type, raw_data, context
                );
                
                PersistenceBundle bundle;
                bundle.processed_data = processed_data;
                bundle.checkpoints = relevant_checkpoints;
                bundle.resource_type = decision.resource_type;
                
                if (ufs_adapter->persist_data_bundle(bundle)) {
                    cache->remove_data_before_timestamp(
                        decision.resource_type, 
                        decision.evict_before_timestamp
                    );
                    
                    cache->remove_checkpoints_before(
                        decision.resource_type, 
                        decision.evict_before_timestamp
                    );
                    
                    log_info("Successfully processed and persisted data");
                } else {
                    log_error("Failed to persist data bundle");
                }
                
            } catch (const std::exception& e) {
                log_error("Error processing persistence decision: " + std::string(e.what()));
            }
        }
    }
};
```

### 4 DataStoreStrategy 
``` c++
class DataStoreStrategy {
    virtual ~DataStoreStrategy() = default;
    virtual ProcessedDataResult processtore(const QueryResult& raw_data) = 0;
    virtual std::string get_strategy_name() const = 0;
};

class CameraFrameSubsamplingStrategy : public DataStoreStrategy {
private:
    int subsample_ratio;  // 1表示保留所有帧，2表示保留1/2帧，等等
    
public:
    explicit CameraFrameSubsamplingStrategy(int ratio) : subsample_ratio(ratio) {}
    
    ProcessedDataResult processtore(const QueryResult& raw_data) override {
        // 具体逻辑 比如抽帧
    }
    
    std::string get_strategy_name() const override {
        return "camera_frame_subsampling_" + std::to_string(subsample_ratio);
    }
};

class AudioDownsamplingStrategy : public DataStoreStrategy {
private:
    int downsample_factor;
    
public:
    explicit AudioDownsamplingStrategy(int factor) : downsample_factor(factor) {}
    
    ProcessedDataResult processtore(const QueryResult& raw_data) override {
        //具体逻辑，例如可以downsample_audio
        return result;
    }
    
    std::string get_strategy_name() const override {
        return "audio_downsampling_" + std::to_string(downsample_factor);
    }

private:
    std::vector<uint8_t> downsample_audio(const std::vector<uint8_t>& audio_data, int factor) {
        // 实现音频降采样逻辑
        std::vector<uint8_t> downsampled;
        // ... 降采样实现
        return downsampled;
    }
};

class CheckpointCompressionStrategy : public DataStoreStrategy {
private:
    std::chrono::milliseconds checkpoint_interval;
    
public:
    explicit CheckpointCompressionStrategy(std::chrono::milliseconds interval) 
        : checkpoint_interval(interval) {}
    
    ProcessedDataResult process(const QueryResult& raw_data) override {
        //... 具体的数据精简逻辑
    }
    
    std::string get_strategy_name() const override {
        return "checkpoint_compression_" + std::to_string(checkpoint_interval.count()) + "ms";
    }
};

class DataStoreStrategyFactory {
public:
    static std::unique_ptr<DataProcessingStrategy> create_strategy(
        ResourceType type, const ProcessingConfig& config) {
        
        switch (type) {
            case ResourceType::CAMERA:
                return std::make_unique<CameraFrameSubsamplingStrategy>(config.camera_subsample_ratio);
                
            case ResourceType::AUDIO:
                return std::make_unique<AudioDownsamplingStrategy>(config.audio_downsample_factor);
                
            case ResourceType::VEHICLE_SIGNAL:
                return std::make_unique<CheckpointCompressionStrategy>(
                    std::chrono::milliseconds(config.vehicle_signal_checkpoint_interval_ms));
                
            case ResourceType::GPS:
                return std::make_unique<CheckpointCompressionStrategy>(
                    std::chrono::milliseconds(config.gps_checkpoint_interval_ms));
                
            case ResourceType::IMU:
                return std::make_unique<CheckpointCompressionStrategy>(
                    std::chrono::milliseconds(config.imu_checkpoint_interval_ms));
                
            default:
                throw std::runtime_error("Unknown resource type for processing strategy");
        }
    }
};
```

### 4 ConfigurationManager
```c++
struct ProcessingConfig {
    int camera_subsample_ratio = 2;         // Camera帧抽取比例
    int audio_downsample_factor = 2;        // Audio降采样倍数
    int vehicle_signal_checkpoint_interval_ms = 1000;  // 车辆信号检查点间隔
    int gps_checkpoint_interval_ms = 5000;   // GPS检查点间隔
    int imu_checkpoint_interval_ms = 100;    // IMU检查点间隔
};

struct RetentionPolicy {
    std::chrono::milliseconds retention_duration;
    size_t max_cache_size;
    EvictionStrategy eviction_strategy;
};

struct CacheConfiguration {
    std::map<ResourceType, size_t> cache_size_limits;
    std::map<ResourceType, RetentionPolicy> retention_policies;
    ProcessingConfig processing_config;
    int monitoring_interval_seconds = 30;
    
    size_t get_cache_size_limit(ResourceType type) const {
        auto it = cache_size_limits.find(type);
        return it != cache_size_limits.end() ? it->second : DEFAULT_CACHE_SIZE;
    }
    
    RetentionPolicy get_retention_policy(ResourceType type) const {
        auto it = retention_policies.find(type);
        if (it != retention_policies.end()) {
            return it->second;
        }
        // 返回默认策略
        return RetentionPolicy{
            .retention_duration = std::chrono::hours(1),
            .max_cache_size = DEFAULT_CACHE_SIZE,
            .eviction_strategy = EvictionStrategy::LRU
        };
    }

private:
    static constexpr size_t DEFAULT_CACHE_SIZE = 1024 * 1024 * 100; // 100MB
};

class ConfigurationManager {
private:
    CacheConfiguration config;
    std::string config_file_path;
    std::mutex config_mutex;

public:
    explicit ConfigurationManager(const std::string& config_path = "cache_config.json")
        : config_file_path(config_path) {
        load_configuration();
    }

    size_t get_cache_size_limit(ResourceType type) const {
        std::lock_guard<std::mutex> lock(config_mutex);
        return config.get_cache_size_limit(type);
    }

    RetentionPolicy get_retention_policy(ResourceType type) const {
        std::lock_guard<std::mutex> lock(config_mutex);
        return config.get_retention_policy(type);
    }

    ProcessingConfig get_processing_config() const {
        std::lock_guard<std::mutex> lock(config_mutex);
        return config.processing_config;
    }

    int get_monitoring_interval() const {
        std::lock_guard<std::mutex> lock(config_mutex);
        return config.monitoring_interval_seconds;
    }

    // 动态更新配置
    void update_processing_config(const ProcessingConfig& new_config) {
        std::lock_guard<std::mutex> lock(config_mutex);
        config.processing_config = new_config;
        save_configuration();
    }

    void update_cache_size_limit(ResourceType type, size_t new_limit) {
        std::lock_guard<std::mutex> lock(config_mutex);
        config.cache_size_limits[type] = new_limit;
        save_configuration();
    }

    // 重新加载配置文件
    void reload_configuration() {
        std::lock_guard<std::mutex> lock(config_mutex);
        load_configuration();
    }

private:
    void load_configuration() {
        try {
            std::ifstream file(config_file_path);
            if (!file.is_open()) {
                // 如果配置文件不存在，使用默认配置并创建文件
                create_default_configuration();
                return;
            }

            nlohmann::json json_config;
            file >> json_config;
            parse_json_configuration(json_config);

        } catch (const std::exception& e) {
            std::cerr << "Failed to load configuration: " << e.what() 
                      << ". Using default configuration." << std::endl;
            create_default_configuration();
        }
    }

    void parse_json_configuration(const nlohmann::json& json_config) {
        // 解析缓存大小限制
        if (json_config.contains("cache_size_limits")) {
            for (const auto& [type_str, size] : json_config["cache_size_limits"].items()) {
                ResourceType type = string_to_resource_type(type_str);
                config.cache_size_limits[type] = size.get<size_t>();
            }
        }

        // 解析保留策略
        if (json_config.contains("retention_policies")) {
            for (const auto& [type_str, policy_json] : json_config["retention_policies"].items()) {
                ResourceType type = string_to_resource_type(type_str);
                RetentionPolicy policy;
                
                if (policy_json.contains("retention_duration_ms")) {
                    policy.retention_duration = std::chrono::milliseconds(
                        policy_json["retention_duration_ms"].get<int>());
                }
                
                if (policy_json.contains("max_cache_size")) {
                    policy.max_cache_size = policy_json["max_cache_size"].get<size_t>();
                }
                
                if (policy_json.contains("eviction_strategy")) {
                    policy.eviction_strategy = string_to_eviction_strategy(
                        policy_json["eviction_strategy"].get<std::string>());
                }
                
                config.retention_policies[type] = policy;
            }
        }

        // 解析处理配置
        if (json_config.contains("processing_config")) {
            const auto& proc_config = json_config["processing_config"];
            
            if (proc_config.contains("camera_subsample_ratio")) {
                config.processing_config.camera_subsample_ratio = 
                    proc_config["camera_subsample_ratio"].get<int>();
            }
            
            if (proc_config.contains("audio_downsample_factor")) {
                config.processing_config.audio_downsample_factor = 
                    proc_config["audio_downsample_factor"].get<int>();
            }
            
            if (proc_config.contains("vehicle_signal_checkpoint_interval_ms")) {
                config.processing_config.vehicle_signal_checkpoint_interval_ms = 
                    proc_config["vehicle_signal_checkpoint_interval_ms"].get<int>();
            }
            
            if (proc_config.contains("gps_checkpoint_interval_ms")) {
                config.processing_config.gps_checkpoint_interval_ms = 
                    proc_config["gps_checkpoint_interval_ms"].get<int>();
            }
            
            if (proc_config.contains("imu_checkpoint_interval_ms")) {
                config.processing_config.imu_checkpoint_interval_ms = 
                    proc_config["imu_checkpoint_interval_ms"].get<int>();
            }
        }

        // 解析监控间隔
        if (json_config.contains("monitoring_interval_seconds")) {
            config.monitoring_interval_seconds = 
                json_config["monitoring_interval_seconds"].get<int>();
        }
    }

    void create_default_configuration() {
        // 设置默认缓存大小限制
        config.cache_size_limits[ResourceType::CAMERA] = 500 * 1024 * 1024;      // 500MB
        config.cache_size_limits[ResourceType::AUDIO] = 100 * 1024 * 1024;       // 100MB
        config.cache_size_limits[ResourceType::VEHICLE_SIGNAL] = 50 * 1024 * 1024; // 50MB
        config.cache_size_limits[ResourceType::GPS] = 10 * 1024 * 1024;          // 10MB
        config.cache_size_limits[ResourceType::IMU] = 20 * 1024 * 1024;          // 20MB

        // 设置默认保留策略
        config.retention_policies[ResourceType::CAMERA] = RetentionPolicy{
            .retention_duration = std::chrono::minutes(30),
            .max_cache_size = 500 * 1024 * 1024,
            .eviction_strategy = EvictionStrategy::LRU
        };
        
        config.retention_policies[ResourceType::AUDIO] = RetentionPolicy{
            .retention_duration = std::chrono::hours(1),
            .max_cache_size = 100 * 1024 * 1024,
            .eviction_strategy = EvictionStrategy::LRU
        };

        // 设置默认处理配置
        config.processing_config = ProcessingConfig{
            .camera_subsample_ratio = 3,
            .audio_downsample_factor = 2,
            .vehicle_signal_checkpoint_interval_ms = 1000,
            .gps_checkpoint_interval_ms = 5000,
            .imu_checkpoint_interval_ms = 100
        };

        config.monitoring_interval_seconds = 30;

        // 保存默认配置到文件
        save_configuration();
    }

    void save_configuration() {
        try {
            nlohmann::json json_config;

            // 保存缓存大小限制
            nlohmann::json cache_sizes;
            for (const auto& [type, size] : config.cache_size_limits) {
                cache_sizes[resource_type_to_string(type)] = size;
            }
            json_config["cache_size_limits"] = cache_sizes;

            // 保存保留策略
            nlohmann::json retention_policies;
            for (const auto& [type, policy] : config.retention_policies) {
                nlohmann::json policy_json;
                policy_json["retention_duration_ms"] = policy.retention_duration.count();
                policy_json["max_cache_size"] = policy.max_cache_size;
                policy_json["eviction_strategy"] = eviction_strategy_to_string(policy.eviction_strategy);
                retention_policies[resource_type_to_string(type)] = policy_json;
            }
            json_config["retention_policies"] = retention_policies;

            // 保存处理配置
            nlohmann::json proc_config;
            proc_config["camera_subsample_ratio"] = config.processing_config.camera_subsample_ratio;
            proc_config["audio_downsample_factor"] = config.processing_config.audio_downsample_factor;
            proc_config["vehicle_signal_checkpoint_interval_ms"] = config.processing_config.vehicle_signal_checkpoint_interval_ms;
            proc_config["gps_checkpoint_interval_ms"] = config.processing_config.gps_checkpoint_interval_ms;
            proc_config["imu_checkpoint_interval_ms"] = config.processing_config.imu_checkpoint_interval_ms;
            json_config["processing_config"] = proc_config;

            // 保存监控间隔
            json_config["monitoring_interval_seconds"] = config.monitoring_interval_seconds;

            std::ofstream file(config_file_path);
            file << json_config.dump(4);

        } catch (const std::exception& e) {
            std::cerr << "Failed to save configuration: " << e.what() << std::endl;
        }
    }

    ResourceType string_to_resource_type(const std::string& type_str) {
        if (type_str == "CAMERA") return ResourceType::CAMERA;
        if (type_str == "AUDIO") return ResourceType::AUDIO;
        if (type_str == "VEHICLE_SIGNAL") return ResourceType::VEHICLE_SIGNAL;
        if (type_str == "GPS") return ResourceType::GPS;
        if (type_str == "IMU") return ResourceType::IMU;
        throw std::runtime_error("Unknown resource type: " + type_str);
    }

    std::string resource_type_to_string(ResourceType type) {
        switch (type) {
            case ResourceType::CAMERA: return "CAMERA";
            case ResourceType::AUDIO: return "AUDIO";
            case ResourceType::VEHICLE_SIGNAL: return "VEHICLE_SIGNAL";
            case ResourceType::GPS: return "GPS";
            case ResourceType::IMU: return "IMU";
            default: return "UNKNOWN";
        }
    }

    EvictionStrategy string_to_eviction_strategy(const std::string& strategy_str) {
        if (strategy_str == "LRU") return EvictionStrategy::LRU;
        if (strategy_str == "FIFO") return EvictionStrategy::FIFO;
        if (strategy_str == "SIZE_BASED") return EvictionStrategy::SIZE_BASED;
        return EvictionStrategy::LRU; // 默认使用LRU
    }

    std::string eviction_strategy_to_string(EvictionStrategy strategy) {
        switch (strategy) {
            case EvictionStrategy::LRU: return "LRU";
            case EvictionStrategy::FIFO: return "FIFO";
            case EvictionStrategy::SIZE_BASED: return "SIZE_BASED";
            default: return "LRU";
        }
    }
};

```


### 4 UFSAdapter
``` c++
// 文件路径和索引管理
struct FileMetadata {
    std::string file_path;
    ResourceType resource_type;
    CompressionType compression_type;
    long long start_timestamp;
    long long end_timestamp;
    size_t file_size;
    uint32_t checksum;
    std::chrono::time_point<std::chrono::steady_clock> created_time;
    uint32_t write_count;  // 写入次数，用于磨损均衡
};

// 存储目录结构配置
struct StorageConfig {
    std::string base_path = "/data/timeline_cache";  // 车载系统数据目录
    size_t max_storage_size_mb = 2048;               // 最大存储空间 2GB
    size_t max_files_per_dir = 1000;                 // 每个目录最大文件数
    size_t gc_threshold_percent = 85;                // GC触发阈值
    size_t max_file_size_mb = 64;                    // 单文件最大大小
    bool enable_wear_leveling = true;                // 启用磨损均衡
    int sync_interval_seconds = 30;                  // 数据同步间隔
};

class UfsAdapter {
private:
    StorageConfig config;
    std::mutex file_index_mutex;
    std::map<std::string, FileMetadata> file_index;  // 文件索引
    std::map<ResourceType, std::string> resource_dirs; // 资源类型对应的目录
    std::unique_ptr<StorageGarbageCollector> gc;
    std::unique_ptr<WearLevelingManager> wear_manager;
    std::thread sync_thread;
    std::atomic<bool> running{false};
    
    // 当前写入文件句柄缓存
    std::map<ResourceType, std::pair<std::ofstream, std::string>> active_writers;
    std::mutex writers_mutex;

public:
    //持久化数据
    bool persist_data(ResourceType type, const QueryResult& query_result) {
        if (query_result.packets.empty()) {
            return true;  // 没有数据需要持久化
        }
        
        bool success = true;
        for (const auto& packet : query_result.packets) {
            // 根据数据类型选择合适的写入方法
            if (should_compress(type, packet.data.size())) {
                auto compressed_data = compress_data(packet.data);
                success &= write_compressed_data(packet.timestamp, type, compressed_data);
            } else {
                success &= write_plain_data(packet.timestamp, type, packet.data);
            }
        }
        return success;
    }

    
    // 写入普通数据
    bool write_plain_data(long long timestamp, ResourceType type, const std::vector<uint8_t>& data) {
        try {
            std::string file_path = get_or_create_active_file(type, timestamp);
            
            // 构建数据包头
            DataPacketHeader header;
            header.timestamp = timestamp;
            header.data_size = data.size();
            header.compression_type = CompressionType::NONE;
            header.checksum = calculate_crc32(data.data(), data.size());
            
            std::lock_guard<std::mutex> lock(writers_mutex);
            auto& writer_pair = active_writers[type];
            auto& writer = writer_pair.first;
            
            // 写入包头和数据
            writer.write(reinterpret_cast<const char*>(&header), sizeof(header));
            writer.write(reinterpret_cast<const char*>(data.data()), data.size());
            
            // 定期flush确保数据安全
            if (timestamp % 10000 == 0) {  // 每10秒flush一次
                writer.flush();
                fsync(get_file_descriptor(writer));
            }
            
            update_file_metadata(file_path, type, timestamp, data.size());
            
            return true;
            
        } catch (const std::exception& e) {
            log_error("Failed to write plain data: " + std::string(e.what()));
            return false;
        }
    }
    
    // 写入压缩数据
    bool write_compressed_data(long long timestamp, ResourceType type, const std::vector<uint8_t>& compressed_data) {
        try {
            std::string file_path = get_or_create_active_file(type, timestamp);
            
            DataPacketHeader header;
            header.timestamp = timestamp;
            header.data_size = compressed_data.size();
            header.compression_type = CompressionType::LZ4;  // 假设使用LZ4压缩
            header.checksum = calculate_crc32(compressed_data.data(), compressed_data.size());
            
            std::lock_guard<std::mutex> lock(writers_mutex);
            auto& writer_pair = active_writers[type];
            auto& writer = writer_pair.first;
            
            writer.write(reinterpret_cast<const char*>(&header), sizeof(header));
            writer.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
            
            if (timestamp % 10000 == 0) {
                writer.flush();
                fsync(get_file_descriptor(writer));
            }
            
            update_file_metadata(file_path, type, timestamp, compressed_data.size());
            
            return true;
            
        } catch (const std::exception& e) {
            log_error("Failed to write compressed data: " + std::string(e.what()));
            return false;
        }
    }
    
    // 读取指定时间范围的数据
    std::vector<PersistedDataInfo> read_data_in_range(const TimeRange& range) {
        std::vector<PersistedDataInfo> result;
        
        std::lock_guard<std::mutex> lock(file_index_mutex);
        
        for (const auto& [file_path, metadata] : file_index) {
            // 检查时间范围重叠
            if (metadata.end_timestamp >= range.start_ts && metadata.start_timestamp <= range.end_ts) {
                
                if (metadata.compression_type == CompressionType::NONE && 
                    file_path.find(".idx") == std::string::npos) {
                    // 普通数据文件，需要扫描读取
                    auto data_infos = scan_data_file(file_path, range);
                    result.insert(result.end(), data_infos.begin(), data_infos.end());
                } else {
                    // 索引数据文件，直接使用索引
                    auto data_infos = read_indexed_data_range(file_path, range);
                    result.insert(result.end(), data_infos.begin(), data_infos.end());
                }
            }
        }
        
        // 按时间戳排序
        std::sort(result.begin(), result.end(), 
            [](const PersistedDataInfo& a, const PersistedDataInfo& b) {
                return a.timestamp < b.timestamp;
            });
        
        return result;
    }
    
    // 读取普通数据
    std::vector<uint8_t> read_plain_data(const PersistedDataInfo& info) {
        try {
            std::ifstream file(info.file_path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open file: " + info.file_path);
            }
            
            file.seekg(info.file_offset);
            
            // 读取包头
            DataPacketHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(header));
            
            if (header.timestamp != info.timestamp) {
                throw std::runtime_error("Timestamp mismatch in file");
            }
            
            // 读取数据
            std::vector<uint8_t> data(header.data_size);
            file.read(reinterpret_cast<char*>(data.data()), header.data_size);
            
            // 验证校验和
            uint32_t calculated_checksum = calculate_crc32(data.data(), data.size());
            if (calculated_checksum != header.checksum) {
                throw std::runtime_error("Data corruption detected");
            }
            
            return data;
            
        } catch (const std::exception& e) {
            log_error("Failed to read plain data: " + std::string(e.what()));
            return {};
        }
    }
    
    // 读取压缩数据
    std::vector<uint8_t> read_compressed_data(const PersistedDataInfo& info) {
        // 实现与read_plain_data类似，但返回的是压缩数据
        // 调用方负责解压缩
        return read_plain_data(info);  // 简化实现
    }

private:
    // 初始化存储目录结构
    void initialize_storage_structure() {
        try {
            // 创建基础目录
            std::filesystem::create_directories(config.base_path);
            
            // 为每种资源类型创建子目录
            resource_dirs[ResourceType::CAMERA] = config.base_path + "/camera";
            resource_dirs[ResourceType::VEHICLE_SIGNAL] = config.base_path + "/vehicle_signal";
            resource_dirs[ResourceType::GPS] = config.base_path + "/gps";
            resource_dirs[ResourceType::IMU] = config.base_path + "/imu";
            
            for (const auto& [type, dir_path] : resource_dirs) {
                std::filesystem::create_directories(dir_path);
                std::filesystem::create_directories(dir_path + "/archive");  // 归档目录
            }
            
            // 创建索引目录
            std::filesystem::create_directories(config.base_path + "/index");
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to initialize storage structure: " + std::string(e.what()));
        }
    }
    
    // 加载文件索引
    void load_file_index() {
        std::string index_file = config.base_path + "/index/file_index.dat";
        
        if (!std::filesystem::exists(index_file)) {
            // 首次运行，扫描现有文件构建索引
            rebuild_file_index();
            return;
        }
        
        try {
            std::ifstream file(index_file, std::ios::binary);
            if (!file) return;
            
            std::lock_guard<std::mutex> lock(file_index_mutex);
            
            size_t entry_count;
            file.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));
            
            for (size_t i = 0; i < entry_count; i++) {
                FileMetadata metadata;
                
                // 读取文件路径长度和路径
                size_t path_length;
                file.read(reinterpret_cast<char*>(&path_length), sizeof(path_length));
                
                std::string file_path(path_length, '\0');
                file.read(file_path.data(), path_length);
                
                // 读取元数据
                file.read(reinterpret_cast<char*>(&metadata.resource_type), sizeof(metadata.resource_type));
                file.read(reinterpret_cast<char*>(&metadata.compression_type), sizeof(metadata.compression_type));
                file.read(reinterpret_cast<char*>(&metadata.start_timestamp), sizeof(metadata.start_timestamp));
                file.read(reinterpret_cast<char*>(&metadata.end_timestamp), sizeof(metadata.end_timestamp));
                file.read(reinterpret_cast<char*>(&metadata.file_size), sizeof(metadata.file_size));
                file.read(reinterpret_cast<char*>(&metadata.checksum), sizeof(metadata.checksum));
                file.read(reinterpret_cast<char*>(&metadata.write_count), sizeof(metadata.write_count));
                
                metadata.file_path = file_path;
                metadata.created_time = std::chrono::steady_clock::now();
                
                // 验证文件是否仍然存在
                if (std::filesystem::exists(file_path)) {
                    file_index[file_path] = std::move(metadata);
                }
            }
            
        } catch (const std::exception& e) {
            log_error("Failed to load file index, rebuilding: " + std::string(e.what()));
            rebuild_file_index();
        }
    }
    
    // 重建文件索引
    void rebuild_file_index() {
        std::lock_guard<std::mutex> lock(file_index_mutex);
        file_index.clear();
        
        for (const auto& [type, dir_path] : resource_dirs) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
                    if (entry.is_regular_file() && entry.path().extension() != ".idx") {
                        analyze_and_index_file(entry.path().string(), type);
                    }
                }
            } catch (const std::exception& e) {
                log_error("Failed to scan directory " + dir_path + ": " + std::string(e.what()));
            }
        }
        
        // 保存重建的索引
        save_file_index();
    }
    
    // 分析文件并添加到索引
    void analyze_and_index_file(const std::string& file_path, ResourceType type) {
        try {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) return;
            
            FileMetadata metadata;
            metadata.file_path = file_path;
            metadata.resource_type = type;
            metadata.file_size = std::filesystem::file_size(file_path);
            metadata.created_time = std::chrono::steady_clock::now();
            metadata.write_count = 0;
            
            // 读取第一个和最后一个数据包来确定时间范围
            DataPacketHeader first_header, last_header;
            
            // 读取第一个包头
            file.read(reinterpret_cast<char*>(&first_header), sizeof(first_header));
            if (file.gcount() == sizeof(first_header)) {
                metadata.start_timestamp = first_header.timestamp;
                metadata.compression_type = first_header.compression_type;
                
                // 寻找最后一个包头
                file.seekg(-static_cast<long>(sizeof(DataPacketHeader)), std::ios::end);
                long current_pos = file.tellg();
                
                while (current_pos > sizeof(DataPacketHeader)) {
                    file.seekg(current_pos);
                    file.read(reinterpret_cast<char*>(&last_header), sizeof(last_header));
                    
                    if (last_header.timestamp >= first_header.timestamp && 
                        last_header.data_size < 100 * 1024 * 1024) {  // 合理性检查
                        metadata.end_timestamp = last_header.timestamp;
                        break;
                    }
                    
                    current_pos -= 1024;  // 向前搜索
                }
                
                if (metadata.end_timestamp == 0) {
                    metadata.end_timestamp = metadata.start_timestamp;
                }
            }
            
            file_index[file_path] = std::move(metadata);
            
        } catch (const std::exception& e) {
            log_error("Failed to analyze file " + file_path + ": " + std::string(e.what()));
        }
    }
    
    // 获取或创建活动写入文件
    std::string get_or_create_active_file(ResourceType type, long long timestamp) {
        std::lock_guard<std::mutex> lock(writers_mutex);
        
        auto it = active_writers.find(type);
        if (it != active_writers.end()) {
            auto& [writer, file_path] = it->second;
            
            // 检查当前文件是否需要轮转
            if (should_rotate_file(file_path, writer.tellp())) {
                writer.close();
                active_writers.erase(it);
            } else {
                return file_path;
            }
        }
        
        // 创建新文件
        std::string new_file_path = generate_new_filename(type, timestamp);
        std::ofstream new_writer(new_file_path, std::ios::binary | std::ios::app);
        
        if (!new_writer) {
            throw std::runtime_error("Cannot create file: " + new_file_path);
        }
        
        active_writers[type] = std::make_pair(std::move(new_writer), new_file_path);
        return new_file_path;
    }
    
    // 生成新文件名
    std::string generate_new_filename(ResourceType type, long long timestamp) {
        auto dir_it = resource_dirs.find(type);
        if (dir_it == resource_dirs.end()) {
            throw std::runtime_error("Unknown resource type");
        }
        
        // 使用时间戳和序列号生成文件名
        auto time_t = timestamp / 1000;
        auto tm = *std::localtime(&time_t);
        
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/data_%04d%02d%02d_%02d%02d%02d_%lld.dat",
                dir_it->second.c_str(),
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec,
                timestamp);
        
        return std::string(filename);
    }
    
    // 生成索引文件名（用于大数据）
    std::string generate_indexed_filename(ResourceType type, long long timestamp) {
        auto dir_it = resource_dirs.find(type);
        if (dir_it == resource_dirs.end()) {
            throw std::runtime_error("Unknown resource type");
        }
        
        auto time_t = timestamp / 1000;
        auto tm = *std::localtime(&time_t);
        
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/indexed_%04d%02d%02d_%02d%02d%02d_%lld.dat",
                dir_it->second.c_str(),
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec,
                timestamp);
        
        return std::string(filename);
    }
    
    // 判断是否需要轮转文件
    bool should_rotate_file(const std::string& file_path, std::streampos current_size) {
        size_t max_size = config.max_file_size_mb * 1024 * 1024;
        return static_cast<size_t>(current_size) >= max_size;
    }
    
    // 扫描数据文件获取指定范围的数据
    std::vector<PersistedDataInfo> scan_data_file(const std::string& file_path, const TimeRange& range) {
        std::vector<PersistedDataInfo> result;
        
        try {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) return result;
            
            long long offset = 0;
            DataPacketHeader header;
            
            while (file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
                if (header.timestamp >= range.start_ts && header.timestamp <= range.end_ts) {
                    PersistedDataInfo info;
                    info.timestamp = header.timestamp;
                    info.file_path = file_path;
                    info.file_offset = offset;
                    info.data_size = header.data_size;
                    info.compression_type = header.compression_type;
                    info.checksum = header.checksum;
                    
                    result.push_back(info);
                }
                
                // 跳过数据部分
                file.seekg(header.data_size, std::ios::cur);
                offset = file.tellg();
            }
            
        } catch (const std::exception& e) {
            log_error("Failed to scan data file " + file_path + ": " + std::string(e.what()));
        }
        
        return result;
    }
    
    // 启动后台任务
    void start_background_tasks() {
        running = true;
        sync_thread = std::thread(&UfsAdapter::background_sync_task, this);
    }
    
    // 后台同步任务
    void background_sync_task() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(config.sync_interval_seconds));
            
            if (!running) break;
            
            try {
                // 定期同步文件索引
                save_file_index();
                
                // 触发垃圾回收检查
                gc->check_and_cleanup();
                
                // 磨损均衡检查
                if (config.enable_wear_leveling) {
                    wear_manager->balance_if_needed();
                }
                
                // flush所有活动写入器
                flush_all_writers();
                
            } catch (const std::exception& e) {
                log_error("Background sync task error: " + std::string(e.what()));
            }
        }
    }
    
    // 关闭适配器
    void shutdown() {
        running = false;
        
        if (sync_thread.joinable()) {
            sync_thread.join();
        }
        
        // 关闭所有写入器
        std::lock_guard<std::mutex> lock(writers_mutex);
        for (auto& [type, writer_pair] : active_writers) {
            writer_pair.first.close();
        }
        active_writers.clear();
        
        // 保存文件索引
        save_file_index();
    }
    
    // 工具函数
    uint32_t calculate_crc32(const uint8_t* data, size_t size) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < size; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
        return ~crc;
    }
    
    void sync_file_to_disk(const std::string& file_path) {
        int fd = open(file_path.c_str(), O_RDONLY);
        if (fd >= 0) {
            fsync(fd);
            close(fd);
        }
    }
    
    int get_file_descriptor(const std::ofstream& stream) {
        // 需要使用平台相关的方法获取文件描述符
        // 这里是简化实现
        return -1;  // 实际实现中需要正确获取fd
    }
    
    void flush_all_writers() {
        std::lock_guard<std::mutex> lock(writers_mutex);
        for (auto& [type, writer_pair] : active_writers) {
            writer_pair.first.flush();
        }
    }
    
    void save_file_index() {
        std::string index_file = config.base_path + "/index/file_index.dat";
        std::string temp_file = index_file + ".tmp";
        
        try {
            std::ofstream file(temp_file, std::ios::binary);
            if (!file) return;
            
            std::lock_guard<std::mutex> lock(file_index_mutex);
            
            size_t entry_count = file_index.size();
            file.write(reinterpret_cast<const char*>(&entry_count), sizeof(entry_count));
            
            for (const auto& [file_path, metadata] : file_index) {
                size_t path_length = file_path.length();
                file.write(reinterpret_cast<const char*>(&path_length), sizeof(path_length));
                file.write(file_path.data(), path_length);
                
                file.write(reinterpret_cast<const char*>(&metadata.resource_type), sizeof(metadata.resource_type));
                file.write(reinterpret_cast<const char*>(&metadata.compression_type), sizeof(metadata.compression_type));
                file.write(reinterpret_cast<const char*>(&metadata.start_timestamp), sizeof(metadata.start_timestamp));
                file.write(reinterpret_cast<const char*>(&metadata.end_timestamp), sizeof(metadata.end_timestamp));
                file.write(reinterpret_cast<const char*>(&metadata.file_size), sizeof(metadata.file_size));
                file.write(reinterpret_cast<const char*>(&metadata.checksum), sizeof(metadata.checksum));
                file.write(reinterpret_cast<const char*>(&metadata.write_count), sizeof(metadata.write_count));
            }
            
            file.close();
            
            // 原子性替换
            std::filesystem::rename(temp_file, index_file);
            sync_file_to_disk(index_file);
            
        } catch (const std::exception& e) {
            log_error("Failed to save file index: " + std::string(e.what()));
            std::filesystem::remove(temp_file);
        }
    }
};


```