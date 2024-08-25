//
// Created by ynachi on 8/17/24.
//

#ifndef SERVER_HH
#define SERVER_HH

#include <boost/asio.hpp>

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::detached;
    using boost::asio::ip::tcp;

    class Server
    {
    public:
        Server(const tcp::endpoint &endpoint, bool reuse_addr);

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept;

        Server &operator=(Server &&) noexcept;

        awaitable<void> listen();

        void run()
        {
            co_spawn(_io_ctx, listen(), detached);
            _io_ctx.run();
        }

    private:
        boost::asio::io_context _io_ctx;
        boost::asio::ip::tcp::acceptor _acceptor;
    };
}  // namespace redis

#endif  // SERVER_HH
