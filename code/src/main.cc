#include "MultiResourceTimelineCache.h"
#include "DataIngestionManager.h"

#include <iostream>

int main() {
    MultiResourceTimelineCache mrtc{CacheConfiguration()};
    DataIngestionManager dm{&mrtc};
    char str[3]{"ab"};
    for (int i = 0; i < 1000; ++i) {
        str[1] = 'a' + i % 26;
        dm.ingestData({i, ResourceType::CAMERA, str, 2});
    }
    std::cout << "Ingest End" << std::endl;
    std::unique_ptr<BaseQuery> query = std::make_unique<CameraQuery>(&mrtc);
    for (int i = 0; i < 1000; i += 10) {
        QueryRequest request;
        request.queryType = QueryType::Timestamp;
        request.parameters["timestamp"] = i;
        auto ret = query->handleQuery(request);
        for (auto c : ret.data) {
            std::cout << c << " ";
        }
        std::cout << std::endl;
    }
}