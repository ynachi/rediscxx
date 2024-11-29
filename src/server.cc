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
            // photon::thread_yield();
        }
    }

    // void Server::handle_connection(std::unique_ptr<photon::net::ISocketStream> conn)
    // {
    //     auto handler = Handler(std::move(conn), 2048);
    //     while (true)
    //     {
    //         auto num_read = handler.read_more_from_stream();
    //         if (num_read < 0)
    //         {
    //             LOG_ERRNO_RETURN(0, , "failed to read from tcp socket");
    //         }
    //         auto ans = handler.read_simple_frame();
    //         if (!ans.has_value())
    //         {
    //             LOG_ERRNO_RETURN(0, , "failed to decode frame");
    //         }
    //         // Echo the received message back to the client
    //         auto frame = ans.value().to_string();
    //         conn->send(frame.data(), frame.size());
    //         LOG_INFO("Received and sent back ", frame.size(), " bytes.");
    //     }
    // }
    void Server::handle_connection(std::unique_ptr<photon::net::ISocketStream> stream)
    {
        LOG_INFO("server::handle_connection on vcpu ", photon::get_vcpu_num());
        std::vector<char> buf;  // Predefined buffer size
        buf.reserve(1024);
        while (true)
        {
            ssize_t ret = stream->recv(buf.data(), 1024);
            if (ret == 0)
            {
                LOG_INFO("server::handle_connection received EOF ", photon::get_vcpu_num());
                return;
            }
            if (ret < 0)
            {
                LOG_FATAL("failed to bind tcp socket");
                // LOG_ERRNO_RETURN(0,, "failed to read socket");
            }
            // Echo the received message back to the client
            for (size_t i = 0; i < ret; ++i)
            {
                buf[i] = std::toupper(buf[i]);
            }
            stream->send(buf.data(), ret);
            LOG_INFO("Received and sent back ", ret, " bytes. ", photon::get_vcpu_num());
            photon::thread_yield();
        }
        // LOG_DEBUG("starting a session on vcpu ", photon::get_vcpu_num());
        // for (;;)
        // {
        //     auto maybe_frame = handler.decode(0, 8);
        //     if (!maybe_frame.is_error())
        //     {
        //         auto data = maybe_frame.value().to_string();
        //         handler.write_to_stream(data.data(), data.length());
        //     }
        //     else
        //     {
        //         auto err = maybe_frame.error();
        //         if (err == RedisError::eof)
        //         {
        //             LOG_DEBUG("client disconnected");
        //             return;
        //         }
        //         Frame err_frame{Frame{FrameID::SimpleError, "error while reading frames"}};
        //         handler.write_to_stream(err_frame.to_string().data(), err_frame.to_string().length());
        //     }
        // }
    }


}  // namespace redis
