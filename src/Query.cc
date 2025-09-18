#include "Query.h"
#include "MultiResourceTimelineCache.h"

#include <iomanip>
#include <sstream>


QueryResult BaseQuery::handle_query(QueryRequest& request) {
    // 步骤1: 解析四种接口格式为统一的native格式
    InterfaceParser::parse_request(request);

    // 步骤2: 执行统一的查询逻辑
    // auto native_response = execute_native_query(request);

    return execute_native_query(request);
    
    // if (!native_response.success) {
        // 这里为什么要传入接口类型？query完成之后我们还需要关注接口类型吗？
        // 这里为什么要设计convert_to_final_response作为数据成员？他的设计貌似也不是一个函数对象
        // 为什么要设计FinalQueryResponse和NativeQueryResponse两个数据结构？
        // 能不能合为一个？
        // return convert_to_final_response(native_response, original_request.interface_type);
        // return FinalQueryResponse(native_response);
    // }

    // 步骤3: 判断是否需要后处理
    // if (native_request.need_post_processing) {
    //     native_response.query_result = data_processor->process(
    //         native_response.query_result,
    //         native_request.post_processing_type,
    //         native_request.device_type,
    //         native_request.post_processing_params
    //     );
    // }

    // 步骤4: 格式化响应
    // return convert_to_final_response(native_response, original_request.interface_type);   
    // return FinalQueryResponse(native_response);
} 

QueryResult CameraQuery::execute_native_query(const QueryRequest& request) {
        QueryResult response;
        
        try {
            if (request.query_type == "time_range") {
                // auto start_time = std::get<Timestamp>(request.parameters.at("start_time"));
                // auto end_time = std::get<Timestamp>(request.parameters.at("end_time"));
                auto start_time = request.
                // 这边我目前没发现有什么必须要用queryresult和UniversalQueryResult两个数据结构的必要
                // 目前把他整合为一个 
                // auto raw_result = cache->query_by_range(ResourceType::CAMERA, start_time, end_time);
                // response.query_result = convert_to_universal_result(raw_result, ResourceType::CAMERA);
                response.query_result = cache->query_by_range(ResourceType::CAMERA, start_time, end_time);
                response.success = true;
                
            } else if (request.query_type == "timestamp") {
                auto timestamp = std::get<Timestamp>(request.parameters.at("timestamp"));
                
                // auto raw_result = cache->query_by_range(ResourceType::CAMERA, timestamp - 100, timestamp + 100);
                // response.query_result = convert_to_universal_result(raw_result, ResourceType::CAMERA);
                response.query_result = cache->query_by_range(ResourceType::CAMERA, timestamp - 100, timestamp + 100);
                response.success = true;
            }
            
        } catch (const std::exception& e) {
            response.success = false;
            response.error_message = "Camera query error: " + std::string(e.what());
        }
        return response;
}

NativeQueryResponse VehicleSignalQuery::execute_native_query(const NativeQueryRequest& request) {
    NativeQueryResponse response;

    try {
        if (request.query_type == "time_range") {
            auto start_time = std::get<Timestamp>(request.parameters.at("start_time"));
            auto end_time = std::get<Timestamp>(request.parameters.at("end_time"));
            
            // auto raw_result = cache->query_by_range(ResourceType::VEHICLE_SIGNAL, start_time, end_time);
            // response.query_result = convert_to_universal_result(raw_result, ResourceType::VEHICLE_SIGNAL);
            response.query_result = cache->query_by_range(ResourceType::VEHICLE_SIGNAL, start_time, end_time);
            response.success = true;
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Vehicle signal query error: " + std::string(e.what());
    }
    
    return response;
}

void InterfaceParser::parse_request(QueryRequest& original_request) {
    // 如果是Native，不需要返回
    if (original_request.interface_type == InterfaceType::NATIVE) {
        return;
    }
    // NativeQueryRequest native_request;
    // bool parse_success = false;
    
    switch (original_request.interface_type) {
        case InterfaceType::IPC:
            parse_ipc_request(original_request);
            break;
        case InterfaceType::RPC:
            parse_rpc_request(original_request);
            break;
        case InterfaceType::SQL:
            parse_sql_request(original_request);
            break;
    }
    
    // 解析后处理需求
    // if (parse_success) {
        // determine_post_processing_needs(native_request);
    // }
    // 
    // return {native_request, parse_success};
}

// void InterfaceParser::determine_post_processing_needs(NativeQueryRequest& native_req) {
//     // 规则1: camera + text_only参数 -> 需要文本后处理
//     if (native_req.device_type == "camera") {
//         auto text_only_it = native_req.parameters.find("text_only");
//         if (text_only_it != native_req.parameters.end() && 
//             std::get<std::string>(text_only_it->second) == "true") {
//             native_req.need_post_processing = true;
//             native_req.post_processing_type = "text_only";
//         }
//     }
    
//     // 规则2: vehicle_signal + explainable参数 -> 需要可解释性后处理
//     if (native_req.device_type == "vehicle_signal") {
//         auto explainable_it = native_req.parameters.find("explainable");
//         if (explainable_it != native_req.parameters.end() && 
//             std::get<std::string>(explainable_it->second) == "true") {
//             native_req.need_post_processing = true;
//             native_req.post_processing_type = "explainable";
//         }
//     }
    
//     // 规则3: 根据options判断
//     if (native_req.options.require_explanation) {
//         native_req.need_post_processing = true;
//         if (native_req.post_processing_type.empty()) {
//             native_req.post_processing_type = "explainable";
//         }
//     }
// }

// std::vector<std::string> InterfaceParser::split_string(const std::string& str, char delimiter) {
//     std::vector<std::string> result;
//     std::stringstream ss(str);
//     std::string item;
//     while (std::getline(ss, item, delimiter)) {
//         result.push_back(item);
//     }
//     return result;
// }

// std::variant<long long, std::string, double> InterfaceParser::parse_value(const std::string& value) {
//     // 尝试解析为数字
//     try {
//         if (value.find('.') != std::string::npos) {
//             return std::stod(value);
//         } else {
//             return std::stoll(value);
//         }
//     } catch (...) {
//         return value;  // 作为字符串返回
//     }
// }

// QueryResult QueryDataProcessor::process(
//                                             const QueryResult& raw_result, 
//                                             const std::string& processing_type,
//                                             const std::string& device_type,
//                                             const std::map<std::string, std::string>& params
//                                         ) {
//     QueryResult processed_result{};  // 复制原始结果
    
//     try {
//         // 1. 根据设备类型和处理类型选择处理策略
//         if (device_type == "Vehicle" && processing_type == "Explainable") {
//             processed_result = process_vehicle_explainable(raw_result, params);
//         }
//         else if (device_type == "Camera" && processing_type == "ImageAnalysis") {
//             processed_result = process_camera_image_analysis(raw_result, params);
//         }
//         else if (device_type == "Audio" && processing_type == "Transcription") {
//             processed_result = process_audio_transcription(raw_result, params);
//         }
//         else if (processing_type == "Compression") {
//             processed_result = process_data_compression(raw_result, params);
//         }
//         else if (processing_type == "Format") {
//             processed_result = process_format_conversion(raw_result, params);
//         }
//         else {
//             processed_result = raw_result;  // 复制原始结果
//             // 默认情况：不进行额外处理，直接返回原始结果
//             // log_info("No specific processing required for device: " + device_type + ", processing type: " + processing_type);
//         }
        
//         // 2. 添加处理元信息
//         processed_result.processing_metadata["processed_by"] = "QueryDataProcessor";
//         processed_result.processing_metadata["processing_type"] = processing_type;
//         processed_result.processing_metadata["device_type"] = device_type;
//         processed_result.processing_metadata["processed_at"] = get_current_timestamp_string();
        
//         return processed_result;
        
//     } catch (const std::exception& e) {
//         // log_error("Error in process: " + std::string(e.what()));
//         // 处理失败时返回原始结果，并添加错误信息
//         processed_result.processing_metadata["error"] = e.what();
//         return processed_result;
//     }
// }

// QueryResult QueryDataProcessor::process_vehicle_explainable(const QueryResult& raw_result, 
//                                                  const std::map<std::string, std::string>& params) {
//     QueryResult explainable_result = raw_result;
    
//     // 遍历所有Vehicle Signal类型的数据包
//     for (auto& [resource_type, packets] : explainable_result.resource_data) {
//         if (resource_type == ResourceType::VEHICLE_SIGNAL) {
//             for (auto& packet : packets) {
//                 // 解析原始的Vehicle Signal数据
//                 auto explained_data = explain_vehicle_signal_data(packet);
                
//                 // 替换或扩展原始数据
//                 packet.explained_data = explained_data;
//                 packet.is_explained = true;
//             }
//         }
//     }
    
//     explainable_result.processing_metadata["explanation_applied"] = "true";
//     return explainable_result;
// }

// ExplainedVehicleData QueryDataProcessor::explain_vehicle_signal_data(const UnifiedDataPacket& packet) {
//     ExplainedVehicleData explained;
    
//     // 1. 从数据包中提取原始数值
//     auto raw_values = extract_raw_vehicle_values(packet);
    
//     // 2. 逐个解释每个数值
//     for (const auto& [field_name, raw_value] : raw_values) {
//         ExplanationEntry entry;
//         entry.field_name = field_name;
//         entry.raw_value = raw_value;
        
//         // 3. 根据字段名从配置文件获取解释
//         if (field_name == "camera_status") {
//             entry.explanation = explain_camera_status(raw_value);
//         }
//         else if (field_name == "behavior_code") {
//             entry.explanation = explain_behavior_code(raw_value);
//         }
//         else if (field_name == "gear_position") {
//             entry.explanation = explain_gear_position(raw_value);
//         }
//         else if (field_name == "door_status") {
//             entry.explanation = explain_door_status(raw_value);
//         }
//         else {
//             // 通用解释逻辑：从配置映射中查找
//             entry.explanation = get_explanation_from_config(field_name, raw_value);
//         }
        
//         explained.explanations.push_back(entry);
//     }
    
//     // 4. 生成整体解释摘要
//     explained.summary = generate_vehicle_status_summary(explained.explanations);
    
//     return explained;
// }

// std::string QueryDataProcessor::explain_camera_status(int raw_value) {
//     // 示例：16 -> "Front Camera Active"
//     std::string config_key = "vehicle.camera_status." + std::to_string(raw_value);
    
//     auto it = config_mappings.find(config_key);
//     if (it != config_mappings.end()) {
//         return it->second;
//     }
    
//     // 默认解释
//     switch (raw_value) {
//         case 16: return "Front Camera Active";
//         case 17: return "Rear Camera Active";
//         case 18: return "Left Camera Active";
//         case 19: return "Right Camera Active";
//         case 0:  return "All Cameras Inactive";
//         default: return "Unknown Camera Status (" + std::to_string(raw_value) + ")";
//     }
// }

// std::string QueryDataProcessor::explain_behavior_code(int raw_value) {
//     // 示例：2 -> "Lane Change Detected"
//     std::string config_key = "vehicle.behavior." + std::to_string(raw_value);
    
//     auto it = config_mappings.find(config_key);
//     if (it != config_mappings.end()) {
//         return it->second;
//     }
    
//     // 默认解释
//     switch (raw_value) {
//         case 1: return "Normal Driving";
//         case 2: return "Lane Change Detected";
//         case 3: return "Sudden Braking";
//         case 4: return "Rapid Acceleration";
//         case 5: return "Sharp Turn";
//         default: return "Unknown Behavior (" + std::to_string(raw_value) + ")";
//     }
// }

// std::string QueryDataProcessor::get_explanation_from_config(const std::string& field_name, int raw_value) {
//     std::string config_key = "vehicle." + field_name + "." + std::to_string(raw_value);
    
//     auto it = config_mappings.find(config_key);
//     if (it != config_mappings.end()) {
//         return it->second;
//     }
    
//     return "Raw Value: " + std::to_string(raw_value);
// }

// // **加载配置映射**
// void QueryDataProcessor::load_configuration_mappings() {
//     // 从配置文件加载映射关系
//     // 实际实现中可以从JSON/XML/INI文件读取
    
//     // 摄像头状态映射
//     config_mappings["vehicle.camera_status.16"] = "Front Camera Active";
//     config_mappings["vehicle.camera_status.17"] = "Rear Camera Active";
//     config_mappings["vehicle.camera_status.18"] = "Left Side Camera Active";
//     config_mappings["vehicle.camera_status.19"] = "Right Side Camera Active";
//     config_mappings["vehicle.camera_status.0"] = "All Cameras Inactive";
    
//     // 行为代码映射
//     config_mappings["vehicle.behavior.1"] = "Normal Driving";
//     config_mappings["vehicle.behavior.2"] = "Lane Change Detected";
//     config_mappings["vehicle.behavior.3"] = "Emergency Braking";
//     config_mappings["vehicle.behavior.4"] = "Rapid Acceleration";
//     config_mappings["vehicle.behavior.5"] = "Sharp Turn Left";
//     config_mappings["vehicle.behavior.6"] = "Sharp Turn Right";
    
//     // 档位状态映射
//     config_mappings["vehicle.gear_position.1"] = "Park";
//     config_mappings["vehicle.gear_position.2"] = "Reverse";
//     config_mappings["vehicle.gear_position.3"] = "Neutral";
//     config_mappings["vehicle.gear_position.4"] = "Drive";
//     config_mappings["vehicle.gear_position.5"] = "Sport Mode";
    
//     // 门状态映射（位掩码解释）
//     config_mappings["vehicle.door_status.0"] = "All Doors Closed";
//     config_mappings["vehicle.door_status.1"] = "Driver Door Open";
//     config_mappings["vehicle.door_status.2"] = "Passenger Door Open";
//     config_mappings["vehicle.door_status.4"] = "Rear Left Door Open";
//     config_mappings["vehicle.door_status.8"] = "Rear Right Door Open";
    
//     // log_info("Loaded " + std::to_string(config_mappings.size()) + " configuration mappings");
// }

// std::string QueryDataProcessor::get_explanation_from_config(const std::string& field_name, int raw_value) {
//     std::string config_key = "vehicle." + field_name + "." + std::to_string(raw_value);
    
//     auto it = config_mappings.find(config_key);
//     if (it != config_mappings.end()) {
//         return it->second;
//     }
    
//     return "Raw Value: " + std::to_string(raw_value);
// }

// void QueryDataProcessor::load_configuration_mappings() {

// }

// // **其他处理类型的伪代码实现**
// QueryResult QueryDataProcessor::process_camera_image_analysis(const QueryResult& raw_result, 
//                                                   const std::map<std::string, std::string>& params) {
//     // 图像分析处理
//     // 使用image_analyzer进行物体检测、人脸识别等
//     return raw_result;  // 伪代码
// }

// QueryResult QueryDataProcessor::process_audio_transcription(const QueryResult& raw_result, 
//                                                 const std::map<std::string, std::string>& params) {
//     // 音频转录处理
//     return raw_result;  // 伪代码
// }

// QueryResult QueryDataProcessor::process_format_conversion(const QueryResult& raw_result, 
//                                                 const std::map<std::string, std::string>& params) {
//     // 音频转录处理
//     return raw_result;  // 伪代码
// }

// QueryResult QueryDataProcessor::process_data_compression(const QueryResult& raw_result, 
//                                              const std::map<std::string, std::string>& params) {
//     // 数据压缩处理
//     return raw_result;  // 伪代码
// }

// // **辅助方法**
// std::map<std::string, int> QueryDataProcessor::extract_raw_vehicle_values(const UnifiedDataPacket& packet) {
//     std::map<std::string, int> values;
    
//     // 从packet.data中解析出各个字段的原始数值
//     // 这里假设data是按某种格式存储的结构化数据
//     if (packet.data_size >= 8) {  // 假设至少有4个2字节的值
//         values["camera_status"] = *reinterpret_cast<const int16_t*>(packet.get_ptr());
//         values["behavior_code"] = *reinterpret_cast<const int16_t*>(packet.get_ptr() + 2);
//         values["gear_position"] = *reinterpret_cast<const int16_t*>(packet.get_ptr() + 4);
//         values["door_status"] = *reinterpret_cast<const int16_t*>(packet.get_ptr() + 6);
//     }
    
//     return values;
// }

// std::string QueryDataProcessor::generate_vehicle_status_summary(const std::vector<ExplanationEntry>& explanations) {
//     std::stringstream summary;
//     summary << "Vehicle Status Summary: ";
    
//     for (const auto& entry : explanations) {
//         summary << entry.field_name << "=" << entry.explanation << "; ";
//     }
    
//     return summary.str();
// }

// std::string QueryDataProcessor::get_current_timestamp_string() {
//     auto now = std::chrono::system_clock::now();
//     auto time_t = std::chrono::system_clock::to_time_t(now);
//     std::stringstream ss;
//     ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
//     return ss.str();
// }