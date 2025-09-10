#include "MultiResourceTimelineCache.h"

class QueryDataProcessor;

class ResponseFormatter;

class QueryRequest;

class NativeQueryResponse;

class NativeQueryRequest;

class FinalQueryResponse;

class InterfaceParser;

class BaseQuery {
private:
    MultiResourceTimelineCache* cache;
    std::unique_ptr<QueryDataProcessor> data_processor;
    std::unique_ptr<ResponseFormatter> response_formatter;
public:
    BaseQuery(MultiResourceTimelineCache* c) : cache(c) {
        // need complete
    }

    FinalQueryResponse handle_query(const QueryRequest&);

    virtual ~BaseQuery() = default;
protected:
    virtual NativeQueryResponse execute_native_query(const NativeQueryRequest) = 0;
};