//
// Created by ynachi on 8/17/24.
//

#ifndef SERVER_HH
#define SERVER_HH

#include <seastar/core/future.hh>
#include <seastar/util/log.hh>
#include <utility>

#include "framer/handler.h"

namespace redis
{

    struct ServerConfig
    {
        uint16_t port = 6379;
        std::string address = "127.0.0.1";
        bool reuse_address = true;
        size_t read_buffer_size = 1024;
    };

    class Server
    {
    public:
        // These two methods are useful for the server class to be used as a sharded service
        seastar::future<> stop();

        explicit Server(ServerConfig &&config) : config_(std::move(config)) {}
        // listen to the port and start accepting connections
        seastar::future<> listen();
        seastar::future<> accept_loop(seastar::server_socket &&listener);

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept = delete;

        Server &operator=(Server &&) noexcept = delete;

        static seastar::future<> process_connection(Handler &&handler);

    private:
        size_t thread_number_ = 10;
        ServerConfig config_;
        seastar::lw_shared_ptr<seastar::logger> logger_;
    };
}  // namespace redis

#endif  // SERVER_HH
