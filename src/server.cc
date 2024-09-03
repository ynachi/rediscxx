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

    Server::Server(Private, const tcp::endpoint& endpoint, const bool reuse_addr, const size_t num_threads) :
        _acceptor(_io_ctx, endpoint), thread_number_(num_threads), signals_(_io_ctx, SIGINT, SIGTERM)
    {
        _acceptor.set_option(socket_base::reuse_address(reuse_addr));

        signals_.async_wait(
                [this](boost::system::error_code const&, int)
                {
                    std::cout << "Server::Server: closing connection on signal" << std::endl;
                    _io_ctx.stop();
                });
    }


    awaitable<void> Server::listen()
    {
        for (;;)
        {
            auto socket = co_await this->_acceptor.async_accept(use_awaitable);
            // create a connection object here
            auto const conn = Connection::create(std::move(socket));
            // we use void because we do not want to wait for the future, because connections
            // should be processed as soon as we get them.
            // co_spawn(executor, echo(std::move(socket)), detached);
            co_spawn(_io_ctx, conn->process_frames(), detached);
        }
    }

    void Server::run()
    {
        // let's start by creating a shared reference of the server
        auto self = this->get_ptr();

        // Start listening in a coroutine
        co_spawn(_io_ctx, [self]() -> awaitable<void> { co_await self->listen(); }, detached);

        const size_t system_core_counts = std::thread::hardware_concurrency();
        const size_t num_of_threads =
                this->thread_number_ > system_core_counts ? this->thread_number_ : system_core_counts;

        // Create and run io threads
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < num_of_threads; ++i)
        {
            threads.emplace_back([self]() { self->_io_ctx.run(); });
        }

        // Wait for all threads to complete
        for (auto& thread: threads)
        {
            thread.join();
        }
    }
}  // namespace redis
