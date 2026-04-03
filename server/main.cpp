#include <iostream>
#include <unistd.h>

#include "timeline_cache.h"
#include "server_ipc.h"

int main(int /*argc*/, char*[] /*argv*/)
{
    std::cout << "resource-server starting (with UDS IPC stub)..." << std::endl;

    tl::TimelineCache cache;

    // Insert a demo frame so that tl_query_latest() has data to return.
    tl::ResourceFrame demo;
    demo.timestamp_ns = 1'000;
    demo.source_id    = "demo_source";
    cache.insert(demo);

#ifdef __linux__
    tl::ServerIpc ipc(cache);
    const std::string socket_path = "/tmp/timeline_resource_engine.sock";

    if (!ipc.start(socket_path))
    {
        std::cerr << "Failed to start server IPC on " << socket_path << std::endl;
        return 1;
    }

    std::cout << "Listening on UDS: " << socket_path << std::endl;
    std::cout << "Press Ctrl+C to terminate." << std::endl;

    // Simple blocking loop; in a more complete implementation this would
    // integrate with a proper event loop or signal handling.
    for (;;)
    {
        ::sleep(1);
    }
#else
    std::cout << "UDS IPC is only available on Linux builds." << std::endl;
#endif

    return 0;
}

