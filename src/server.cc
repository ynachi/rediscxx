//
// Created by ynachi on 8/17/24.
//
#include <connection.hh>
#include <server.hh>

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::socket_base;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    Server::Server(const tcp::endpoint &endpoint, const bool reuse_addr) : _io_ctx(), _acceptor(_io_ctx, endpoint)
    {
        _acceptor.set_option(socket_base::reuse_address(reuse_addr));
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
}  // namespace redis
