//
// Created by ynachi on 8/17/24.
//
#include <iostream>
#include <photon/common/alog.h>
#include <server.hh>

#include "framer/handler.h"

namespace redis
{

    Server::Server(const ServerConfig& config) :
        server_config_(config), socket_server_(photon::net::new_tcp_socket_server())
    {
        if (const auto rc = photon::init(config.event_engine_, config.io_engine_); rc != 0)
        {
            LOG_FATAL("photon::init failed");
        };
        if (const auto rc =
                    photon_std::work_pool_init(config.io_thread_count_, config.event_engine_, config.io_engine_);
            rc != 0)
        {
            LOG_FATAL("photon::init io thread pool failed");
        }
        socket_server_->setsockopt<int>(SOL_SOCKET, SO_REUSEADDR, 1);
        LOG_DEBUG("Server::Server initialization finished");
    }

    void Server::run()
    {
        LOG_DEBUG("server::run entering server main loop ", photon::get_vcpu_num());
        if (const auto rc = socket_server_->bind(this->server_config_.port_, this->server_config_.host_); rc != 0)
        {
            LOG_ERRNO_RETURN(0, , "failed to bind tcp socket");
        }

        if (socket_server_->listen() < 0)
        {
            LOG_ERRNO_RETURN(0, , "failed to listen on tcp socket");
        }

        // the worker init fails when it is done in the Constructor. I do not know why yet.
        photon::WorkPool wp(this->server_config_.worker_thread_count_, this->server_config_.event_engine_,
                            photon::INIT_IO_NONE, this->server_config_.max_concurrent_connections_);

        while (true)
        {
            std::unique_ptr<photon::net::ISocketStream> stream(socket_server_->accept());
            if (stream == nullptr)
            {
                LOG_ERRNO_RETURN(0, , "failed to accept tcp socket");
            }
            auto handler = Handler(std::move(stream), this->server_config_.network_read_chunk_);
            wp.async_call(new auto([handler = std::move(handler)]() mutable { handler.start_session(); }));
        }
    }
}  // namespace redis
