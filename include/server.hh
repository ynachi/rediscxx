//
// Created by ynachi on 8/17/24.
//

#ifndef SERVER_HH
#define SERVER_HH

#include <photon/net/socket.h>
#include <photon/thread/std-compat.h>
#include <photon/thread/workerpool.h>

namespace redis
{
    // @TODO: validate IPs and provide a config factory method
    struct ServerConfig
    {
        size_t event_thread_count_ = std::thread::hardware_concurrency();
        size_t event_engine_ = photon::INIT_EVENT_DEFAULT;
        size_t io_engine_ = photon::INIT_IO_NONE;
        photon::net::IPAddr host_{"127.0.0.1"};
        size_t network_read_chunk_{1024};
        uint16_t port_ = 6379;
    };

    class Server : public std::enable_shared_from_this<Server>
    {
    public:
        std::shared_ptr<Server> get_ptr() { return shared_from_this(); }

        explicit Server(const ServerConfig &config);

        ~Server()
        {
            photon_std::work_pool_fini();
            photon::fini();
            delete socket_server_;
        }

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept = delete;

        Server &operator=(Server &&) noexcept = delete;

        void start_accept_loop();

        static void handle_connection(std::unique_ptr<photon::net::ISocketStream> conn);

    private:
        void listen_();

        ServerConfig server_config_{};
        photon::net::ISocketServer *socket_server_ = nullptr;
    };
}  // namespace redis

#endif  // SERVER_HH
