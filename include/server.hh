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

    class Server: public std::enable_shared_from_this<Server>
    {
        struct Private
        {
            explicit Private() = default;
        };

    public:
        std::shared_ptr<Server> get_ptr() { return shared_from_this(); }

        Server(Private, const tcp::endpoint &endpoint, bool reuse_addr, size_t num_threads);

        static std::shared_ptr<Server> create(const tcp::endpoint &endpoint, bool reuse_addr, size_t num_threads)
        {
            return std::make_shared<Server>(Private(), endpoint, reuse_addr, num_threads);
        }

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept;

        Server &operator=(Server &&) noexcept;

        awaitable<void> listen();

        void run();

    private:
        boost::asio::io_context _io_ctx;
        tcp::acceptor _acceptor;
        size_t thread_number_ = 10;
        boost::asio::signal_set signals_;
    };
}  // namespace redis

#endif  // SERVER_HH
