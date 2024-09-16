//
// Created by ynachi on 8/17/24.
//
#include <connection.hh>
#include <iostream>
#include <server.hh>

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::socket_base;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    Server::Server(Private, boost::asio::io_context& io_context, const tcp::endpoint& endpoint, const bool reuse_addr,
                   const size_t num_threads) :
        io_context_(io_context), acceptor_(io_context_, endpoint), thread_number_(num_threads),
        signals_(io_context_, SIGINT, SIGTERM)
    {
        acceptor_.set_option(socket_base::reuse_address(reuse_addr));

        signals_.async_wait(
                [this](boost::system::error_code const&, int)
                {
                    std::cout << "Server::Server: closing connection on signal" << std::endl;
                    io_context_.stop();
                });
    }


    awaitable<void> Server::listen()
    {
        while (!io_context_.stopped())
        {
            try
            {
                auto socket = co_await this->acceptor_.async_accept(use_awaitable);

                auto const conn = Connection::create(std::move(socket));

                co_spawn(io_context_, Server::start_session(conn), detached);
            }
            catch (std::exception& e)
            {
                std::cerr << "Server::Server: server listening error: " << e.what() << std::endl;
            }
        }
        co_return;
    }

    void Server::start()
    {
        // let's start by creating a shared reference of the server
        auto self = this->get_ptr();

        // Start listening in a coroutine
        co_spawn(io_context_, [self]() -> awaitable<void> { co_await self->listen(); }, detached);

        const size_t system_core_counts = std::thread::hardware_concurrency();
        const size_t num_of_threads =
                this->thread_number_ > system_core_counts ? this->thread_number_ : system_core_counts;

        // Create and run io threads
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < num_of_threads; ++i)
        {
            threads.emplace_back([self]() { self->io_context_.run(); });
        }

        // Wait for all threads to complete
        for (auto& thread: threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    awaitable<void> Server::start_session(std::shared_ptr<Connection> conn)  // NOLINT
    {
        co_await conn->process_frames();
    }
}  // namespace redis
