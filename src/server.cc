//
// Created by ynachi on 8/17/24.
//
#include <seastar/core/future-util.hh>
#include <seastar/core/seastar.hh>
#include <seastar/net/api.hh>
#include <server.hh>

#include "framer/handler.h"

namespace redis
{
    seastar::future<> Server::listen()
    {
        seastar::listen_options listen_options;
        listen_options.reuse_address = this->config_.reuse_address;
        listen_options.lba = seastar::server_socket::load_balancing_algorithm::default_;
        try
        {
            auto const listen_address = seastar::make_ipv4_address({this->config_.address, this->config_.port});
            auto listener = seastar::listen(listen_address, listen_options);
            this->logger_->debug("server instance created at {}:{}", this->config_.address, this->config_.port);
            co_await accept_loop(std::move(listener));
        }
        catch (const std::exception& e)
        {
            logger_->error("failed to listen on: {}:{}, error: {}", this->config_.address, this->config_.port,
                           e.what());
            seastar::engine_exit();
        }
    }

    seastar::future<> Server::accept_loop(seastar::server_socket&& listener)
    {
        while (true)
        {
            auto accept_result = co_await listener.accept();
            this->logger_->debug("server accepted a new connection from {}", accept_result.remote_address);

            auto frame_handler =
                    std::make_unique<Handler>(std::move(accept_result.connection), accept_result.remote_address,
                                              this->logger_, this->config_.read_buffer_size);

            // Launch process_connection asynchronously
            (void) seastar::futurize_invoke([frame_handler = std::move(frame_handler)]() mutable -> seastar::future<>
                                            { return process_connection(std::move(*frame_handler)); });

            co_await seastar::maybe_yield();
        }
    }

    seastar::future<> Server::process_connection(Handler&& handler)
    {
        while (true)
        {
            if (handler.eof())
            {
                self->_logger->debug("connection was closed by the user");
                break;
            }
            auto tmp_read_buf = co_await self->_input_stream.read();
            if (tmp_read_buf.empty())
            {
                self->_logger->debug("connection was closed by the user");
                break;
            }
            this->_buffer.add_upstream_data(std::move(tmp_read_buf));
            auto data = this->_buffer.get_simple_string();
            std::cout << data.value() << "\n";
            if (data.error() == FrameDecodeError::Incomplete)
            {
                continue;
            }
            if (!data.has_value())
            {
                self->_logger->debug("error decoding frame");
                continue;
            }
            const auto& ans = data.value();
            std::cout << data.value() << "\n";
            co_await self->_output_stream.write(data.value());
            co_await self->_output_stream.flush();
        }
        co_await self->_output_stream.close();
        co_return;
    }

}  // namespace redis
