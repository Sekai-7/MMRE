#ifndef QUERY_H
#define QUERY_H
// 目前先只考虑支持native方法
// #include "MultiResourceTimelineCache.h"
#include "TimelineCache.h"

struct MultiResourceTimelineCache;

// 标志Query方式
enum class InterfaceType {
    IPC,
    RPC,
    SQL,
    NATIVE
};

enum class QueryType {
    TimeRange,
    Timestamp,
    Event,
    Pattern
};

class ResponseFormatter {};

class ImageAnalyzer;

// 初始传入的Query结构体
struct QueryRequest {
    ResourceType deviceType;           // "camera", "vehicle_signal", "gps", "imu"
    QueryType queryType;            // "time_range", "timestamp", "event", "pattern"
    InterfaceType interfaceType;
    std::unordered_map<std::string, std::variant<Timestamp, std::string>> parameters;
    // 留给非Native使用
    std::string queryPayload;

    bool needPostProcessing = false;
    std::string postProcessingType;  // "text_only", "explainable", "compress", etc.
    std::map<std::string, std::string> postProcessingParams;
};

// Query得到的结构体


// 对Request作解析之后的结构体
// struct NativeQueryRequest {
//     ResourceType type;           // "camera", "vehicle_signal", "gps", "imu"
//     std::string queryType;            // "time_range", "timestamp", "event", "pattern"
//     std::map<std::string, std::variant<Timestamp, std::string, double>> parameters;
//     QueryOptions options;
    
//     // 后处理标识
// };

// struct QueryResult {
    // std::unordered_map<std::string, std::string> processing_metadata;
// };

struct QueryResult {
    ResourceType type;
    Timestamp startTs = -1;
    Timestamp endTs = -1;
 
    // 普通内存模式：聚合在一个 data buffer
    std::vector<uint8_t> data;
 
    // 共享内存模式：可能有多个句柄
    std::vector<SharedMemoryHandle*> shmHandles;
    // std::unordered_map<ResourceType, std::vector<UnifiedDataPacket>> resource_data;
 
    // bool useShm = false;

    // std::unordered_map<std::string, std::string> processing_metadata;
};

// // 内部查询响应
// struct NativeQueryResponse {
//     bool success;
//     std::string errorMessage;
//     QueryResult queryResult;
// };

// // 最终响应（经过后处理）
// struct FinalQueryResponse {
//     bool success;
//     std::string errorMessage;
//     std::string responseData;  // 序列化后的数据
//     std::map<std::string, std::string> metadata;
//     FinalQueryResponse() {}
//     explicit FinalQueryResponse(const NativeQueryResponse&) {}
// }; 

// 暂时先不考虑后处理
// class QueryDataProcessor {
// private:
//     std::map<std::string, std::string> configMappings;
//     std::unique_ptr<ImageAnalyzer> imageAnalyzer;

// public:
//     QueryDataProcessor() {
//         loadConfigurationMappings();
//         imageAnalyzer = std::make_unique<ImageAnalyzer>();
//     }

//     // 统一的后处理入口
//     QueryResult process(   
//                                     const QueryResult& rawResult, 
//                                     const std::string& processingType,
//                                     const std::string& deviceType,
//                                     const std::map<std::string, std::string>& params
//                                 );
// private:
//     // **核心功能：Vehicle Signal的可解释处理**
//     QueryResult processVehicleExplainable(
//                                                         const QueryResult& rawResult, 
//                                                         const std::map<std::string, std::string>& params
//                                                     );
    
//     // **Vehicle Signal数据解释的核心逻辑**
//     ExplainedVehicleData explainVehicleSignalData(const UnifiedDataPacket& packet);
    
//     // **从配置文件读取解释信息**
//     std::string explainCameraStatus(int rawValue);
    
//     std::string explainBehaviorCode(int rawValue);
    
//     // **通用配置查找方法**
//     std::string getExplanationFromConfig(const std::string& fieldName, int rawValue);
    
//     // **加载配置映射**
//     void loadConfigurationMappings();
    
//     // **其他处理类型的伪代码实现**
//     QueryResult processCameraImageAnalysis(const QueryResult& rawResult, const std::map<std::string, std::string>& params);
    
//     QueryResult processAudioTranscription(const QueryResult& rawResult, const std::map<std::string, std::string>& params);
    
//     QueryResult processDataCompression(const QueryResult& rawResult, const std::map<std::string, std::string>& params);

//     QueryResult processFormatConversion(const QueryResult& rawResult, const std::map<std::string, std::string>& params);
    
//     // **辅助方法**
//     std::map<std::string, int> extractRawVehicleValues(const UnifiedDataPacket& packet);
    
//     std::string generateVehicleStatusSummary(const std::vector<ExplanationEntry>& explanations);
    
//     std::string getCurrentTimestampString();
// };




// class InterfaceParser;
class InterfaceParser {
public:
    // 统一的解析入口
    static void parseRequest(QueryRequest& originalRequest);

private:
    static void parseIpcRequest(QueryRequest& request);

    static void parseRpcRequest(QueryRequest& request);

    static void parseSqlRequest(QueryRequest& request);

    static void parseNativeRequest(QueryRequest& request);

    // 确定是否需要后处理, 可以直接在request发起的时候用一个字段来确认，或者扩展为更丰富的规则等等，现在是一个示例。确认之后在格式化的请求中标注。
    // static void determinePostProcessingNeeds(NativeQueryRequest& nativeReq);
    
    // static std::vector<std::string> splitString(const std::string& str, char delimiter);
    
    // static std::variant<long long, std::string, double> parseValue(const std::string& value);
};

class BaseQuery {
protected:
    MultiResourceTimelineCache* cache;
private:
    // std::unique_ptr<QueryDataProcessor> dataProcessor;
    std::unique_ptr<ResponseFormatter> responseFormatter;
public:
    BaseQuery(MultiResourceTimelineCache* c) : cache(c), responseFormatter(std::make_unique<ResponseFormatter>()) {
        // need complete
    }

    QueryResult handleQuery(QueryRequest&);

    virtual ~BaseQuery() = default;
protected:
    virtual QueryResult executeNativeQuery(const QueryRequest&) = 0;
};

class CameraQuery : public BaseQuery {
public:
    CameraQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    QueryResult executeNativeQuery(const QueryRequest& request) override;
};


class VehicleSignalQuery : public BaseQuery {
public:
    VehicleSignalQuery(MultiResourceTimelineCache* c) : BaseQuery(c) {}

protected:
    QueryResult executeNativeQuery(const QueryRequest& request) override;
};

#endif