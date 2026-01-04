#include "Query.h"
#include "MultiResourceTimelineCache.h"

#include <iomanip>
#include <sstream>


QueryResult BaseQuery::handleQuery(QueryRequest& request) {
    InterfaceParser::parseRequest(request);

    return executeNativeQuery(request);
} 

QueryResult CameraQuery::executeNativeQuery(const QueryRequest& request) {
        QueryResult response;

        const auto& parameter = request.parameters;
        
        if (request.queryType == QueryType::TimeRange) {
            auto itStart = parameter.find("start_time");
            auto itEnd = parameter.find("end_time");
            if (itStart != parameter.end() && itEnd != parameter.end()) {
                const auto& startTime = std::get_if<Timestamp>(&itStart->second);
                const auto& endTime = std::get_if<Timestamp>(&itEnd->second);
                if (startTime != nullptr && endTime != nullptr)
                    response = cache->queryByRange(ResourceType::CAMERA, *startTime, *endTime);
            }
        } else if (request.queryType == QueryType::Timestamp) {
            auto itTimestamp = parameter.find("timestamp");
            if (itTimestamp != parameter.end()) {
                const auto& timestamp = std::get_if<Timestamp>(&itTimestamp->second);
                if (timestamp != nullptr)
                    response = cache->query(ResourceType::CAMERA, *timestamp);
            }
        }
            
        return response;
}

QueryResult VehicleSignalQuery::executeNativeQuery(const QueryRequest& request) {
    QueryResult response;

    const auto& parameter = request.parameters;

    if (request.queryType == QueryType::TimeRange) {
        auto itStart = parameter.find("start_time");
        auto itEnd = parameter.find("end_time");
        if (itStart != parameter.end() && itEnd != parameter.end()) {
            const auto& startTime = std::get_if<Timestamp>(&itStart->second);
            const auto& endTime = std::get_if<Timestamp>(&itEnd->second);
            if (startTime != nullptr && endTime != nullptr)
                response = cache->queryByRange(ResourceType::CAMERA, *startTime, *endTime);
        }
    }
    
    return response;
}

void InterfaceParser::parseRequest(QueryRequest& originalRequest) {
    // 如果是Native, 直接返回
    if (originalRequest.interfaceType == InterfaceType::NATIVE) {
        return;
    }
    
    switch (originalRequest.interfaceType) {
        case InterfaceType::IPC:
            parseIpcRequest(originalRequest);
            break;
        case InterfaceType::RPC:
            parseRpcRequest(originalRequest);
            break;
        case InterfaceType::SQL:
            parseSqlRequest(originalRequest);
            break;
    }
}

void InterfaceParser::parseIpcRequest(QueryRequest& request) {

}

void InterfaceParser::parseRpcRequest(QueryRequest& request) {

}

void InterfaceParser::parseSqlRequest(QueryRequest& request) {

}

void InterfaceParser::parseNativeRequest(QueryRequest& request) {

}