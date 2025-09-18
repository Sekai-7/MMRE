// 目前先只考虑支持native方法
// #include "MultiResourceTimelineCache.h"

struct MultiResourceTimelineCache;

// 标志Query方式
enum class InterfaceType {
    IPC,
    RPC,
    SQL,
    NATIVE
};

class ResponseFormatter;

class ImageAnalyzer;

// 初始传入的Query结构体
struct QueryRequest {
    ResourceType device_type;           // "camera", "vehicle_signal", "gps", "imu"
    std::string query_type;            // "time_range", "timestamp", "event", "pattern"
    InterfaceType interface_type;
    // 留给非Native使用
    std::string query_payload;

    bool need_post_processing = false;
    std::string post_processing_type;  // "text_only", "explainable", "compress", etc.
    std::map<std::string, std::string> post_processing_params;
};

// Query得到的结构体


// 对Request作解析之后的结构体
// struct NativeQueryRequest {
//     ResourceType type;           // "camera", "vehicle_signal", "gps", "imu"
//     std::string query_type;            // "time_range", "timestamp", "event", "pattern"
//     std::map<std::string, std::variant<Timestamp, std::string, double>> parameters;
//     QueryOptions options;
    
//     // 后处理标识
// };

// struct QueryResult {
    // std::unordered_map<std::string, std::string> processing_metadata;
// };

struct QueryResult {
    ResourceType type;
    Timestamp start_ts;
    Timestamp end_ts;
 
    // 普通内存模式：聚合在一个 data buffer
    std::vector<uint8_t> data;
 
    // 共享内存模式：可能有多个句柄
    std::vector<std::shared_ptr<SharedMemoryHandle>> shm_handles;
    // std::unordered_map<ResourceType, std::vector<UnifiedDataPacket>> resource_data;
 
    // bool use_shm = false;

    // std::unordered_map<std::string, std::string> processing_metadata;
};

// // 内部查询响应
// struct NativeQueryResponse {
//     bool success;
//     std::string error_message;
//     QueryResult query_result;
// };

// // 最终响应（经过后处理）
// struct FinalQueryResponse {
//     bool success;
//     std::string error_message;
//     std::string response_data;  // 序列化后的数据
//     std::map<std::string, std::string> metadata;
//     FinalQueryResponse() {}
//     explicit FinalQueryResponse(const NativeQueryResponse&) {}
// }; 

// 暂时先不考虑后处理
// class QueryDataProcessor {
// private:
//     std::map<std::string, std::string> config_mappings;
//     std::unique_ptr<ImageAnalyzer> image_analyzer;

// public:
//     QueryDataProcessor() {
//         load_configuration_mappings();
//         image_analyzer = std::make_unique<ImageAnalyzer>();
//     }

//     // 统一的后处理入口
//     QueryResult process(   
//                                     const QueryResult& raw_result, 
//                                     const std::string& processing_type,
//                                     const std::string& device_type,
//                                     const std::map<std::string, std::string>& params
//                                 );
// private:
//     // **核心功能：Vehicle Signal的可解释处理**
//     QueryResult process_vehicle_explainable(
//                                                         const QueryResult& raw_result, 
//                                                         const std::map<std::string, std::string>& params
//                                                     );
    
//     // **Vehicle Signal数据解释的核心逻辑**
//     ExplainedVehicleData explain_vehicle_signal_data(const UnifiedDataPacket& packet);
    
//     // **从配置文件读取解释信息**
//     std::string explain_camera_status(int raw_value);
    
//     std::string explain_behavior_code(int raw_value);
    
//     // **通用配置查找方法**
//     std::string get_explanation_from_config(const std::string& field_name, int raw_value);
    
//     // **加载配置映射**
//     void load_configuration_mappings();
    
//     // **其他处理类型的伪代码实现**
//     QueryResult process_camera_image_analysis(const QueryResult& raw_result, const std::map<std::string, std::string>& params);
    
//     QueryResult process_audio_transcription(const QueryResult& raw_result, const std::map<std::string, std::string>& params);
    
//     QueryResult process_data_compression(const QueryResult& raw_result, const std::map<std::string, std::string>& params);

//     QueryResult process_format_conversion(const QueryResult& raw_result, const std::map<std::string, std::string>& params);
    
//     // **辅助方法**
//     std::map<std::string, int> extract_raw_vehicle_values(const UnifiedDataPacket& packet);
    
//     std::string generate_vehicle_status_summary(const std::vector<ExplanationEntry>& explanations);
    
//     std::string get_current_timestamp_string();
// };




// class InterfaceParser;
class InterfaceParser {
public:
    // 统一的解析入口
    static void parse_request(QueryRequest& original_request);

private:
    static void parse_ipc_request(QueryRequest& request);

    static void parse_rpc_request(QueryRequest& request);

    static void parse_sql_request(QueryRequest& request);

    static void parse_native_request(QueryRequest& request);

    // 确定是否需要后处理, 可以直接在request发起的时候用一个字段来确认，或者扩展为更丰富的规则等等，现在是一个示例。确认之后在格式化的请求中标注。
    // static void determine_post_processing_needs(NativeQueryRequest& native_req);
    
    // static std::vector<std::string> split_string(const std::string& str, char delimiter);
    
    // static std::variant<long long, std::string, double> parse_value(const std::string& value);
};

class BaseQuery {
protected:
    MultiResourceTimelineCache* cache;
private:
    // std::unique_ptr<QueryDataProcessor> data_processor;
    std::unique_ptr<ResponseFormatter> response_formatter;
public:
    BaseQuery(MultiResourceTimelineCache* c) : cache(c) {
        // need complete
    }

    QueryResult handle_query(QueryRequest&);

    virtual ~BaseQuery() = default;
protected:
    virtual QueryResult execute_native_query(const QueryRequest&) = 0;
};

class CameraQuery : public BaseQuery {
public:
    CameraQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    QueryResult execute_native_query(const QueryRequest& request) override;
};


class VehicleSignalQuery : public BaseQuery {
public:
    VehicleSignalQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    QueryResult execute_native_query(const QueryRequest& request) override;
};