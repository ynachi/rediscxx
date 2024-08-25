//
// Created by ynachi on 8/17/24.
//

#ifndef SERVER_HH
#define SERVER_HH

#include <seastar/net/api.hh>
#include <seastar/util/log.hh>

namespace redis {
    class Server {
    public:
        Server(const std::string &ip_addr, u_int16_t port, bool reuse_addr);

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept;

        Server &operator=(Server &&) noexcept;

        seastar::future<> listen();

    private:
        seastar::lw_shared_ptr<seastar::logger> _logger;
        seastar::server_socket _listener;
    };
} // namespace redis

#endif // SERVER_HH
