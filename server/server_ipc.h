#pragma once

#ifdef __linux__

#include <atomic>
#include <string>
#include <thread>

#include "ipc_protocol.h"
#include "timeline_cache.h"

namespace tl
{
    class ServerIpc
    {
    public:
        explicit ServerIpc(TimelineCache& cache);
        ~ServerIpc();

        bool start(const std::string& socket_path);
        void stop();

    private:
        void run_loop();
        void handle_client(int client_fd);

        void handle_query_latest(int client_fd);
        void send_error(int client_fd, const std::string& message);

        int                 listen_fd_{-1};
        std::string         socket_path_;
        std::atomic<bool>   running_{false};
        std::thread         thread_;
        TimelineCache&      cache_;
    };
} // namespace tl

#endif // __linux__

