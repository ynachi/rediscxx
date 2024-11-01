//
// Created by ynachi on 8/17/24.
//
#include <iostream>
#include <photon/common/alog.h>
#include <server.hh>

#include "framer/handler.h"

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
        if (const auto rc = socket_server_->bind(this->server_config_.port_, this->server_config_.host_); rc != 0)
        {
            LOG_ERRNO_RETURN(0, , "failed to bind tcp socket");
        }

        if (socket_server_->listen() < 0)
        {
            LOG_ERRNO_RETURN(0, , "failed to listen on tcp socket");
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
                LOG_ERRNO_RETURN(0, , "failed to accept tcp socket");
            }
            wp.async_call(new auto([stream = std::move(stream)]() mutable { handle_connection(std::move(stream)); }));
            // photon::thread_yield();
        }
    }

    void Server::handle_connection(std::unique_ptr<photon::net::ISocketStream> conn)
    {
        auto handler = Handler(std::move(conn), 2048);
        while (true)
        {
            auto num_read = handler.read_more_from_stream();
            if (num_read < 0)
            {
                LOG_ERRNO_RETURN(0, , "failed to read from tcp socket");
            }
            auto ans = handler.read_simple_frame();
            if (!ans.has_value())
            {
                LOG_ERRNO_RETURN(0, , "failed to decode frame");
            }
            // Echo the received message back to the client
            auto frame = ans.value().to_string();
            conn->send(frame.data(), frame.size());
            LOG_INFO("Received and sent back ", frame.size(), " bytes.");
        }
    }

}  // namespace redis
