//
// Created by ynachi on 8/17/24.
//
#include <iostream>
#include <photon/common/alog.h>
#include <server.hh>

namespace redis
{

    Server::Server(const ServerConfig& config) : server_config_(config)
    {
        if (const auto rc = photon::init(config.event_engine_, config.io_engine_); rc != 0)
        {
            LOG_FATAL("photon::init failed");
        };
        if (const auto rc =
                    photon_std::work_pool_init(config.event_thread_count_, config.event_engine_, config.io_engine_);
            rc != 0)
        {
            LOG_FATAL("photon::init thread pool init failed");
        }

        socket_server_ = photon::net::new_tcp_socket_server();
        if (socket_server_ == nullptr)
        {
            LOG_FATAL("failed to create a tcp socket");
        }
    }


    void Server::listen_()
    {
        if (const auto rc = socket_server_->bind(9527, this->server_config_.host_); rc != 0)
        {
            LOG_FATAL("failed to bind tcp socket");
        }

        if (socket_server_->listen() < 0)
        {
            LOG_FATAL("failed to listen tcp socket");
        }
    }

    void Server::start_accept_loop()
    {
        this->listen_();
        photon::WorkPool wp(std::thread::hardware_concurrency(), photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE,
                            128 * 1024);
        while (true)
        {
            std::unique_ptr<photon::net::ISocketStream> stream(socket_server_->accept());
            if (stream == nullptr)
            {
                LOG_FATAL("failed to accept tcp socket");
            }
            wp.async_call(new auto([stream = std::move(stream)]() mutable { handle_connection(std::move(stream)); }));
            photon::thread_yield();
        }
    }

    void Server::handle_connection(std::unique_ptr<photon::net::ISocketStream> conn)
    {
        std::vector<char> buf;  // Predefined buffer size
        buf.reserve(1024);
        while (true)
        {
            ssize_t ret = conn->recv(buf.data(), 1024);
            if (ret <= 0)
            {
                LOG_FATAL("failed to bind tcp socket");
                // LOG_ERRNO_RETURN(0, , "failed to read socket");
            }
            // Echo the received message back to the client
            for (size_t i = 0; i < ret; ++i)
            {
                buf[i] = std::toupper(buf[i]);
            }
            conn->send(buf.data(), ret);
            LOG_INFO("Received and sent back ", ret, " bytes.");
            photon::thread_yield();
        }
    }

}  // namespace redis
