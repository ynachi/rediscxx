//
// Created by ynachi on 8/17/24.
//

#ifndef SERVER_HH
#define SERVER_HH

#include <boost/asio.hpp>
#include <connection.hh>
#include <iostream>

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::detached;
    using boost::asio::ip::tcp;

    class Server : public std::enable_shared_from_this<Server>
    {
        struct Private
        {
            explicit Private() = default;
        };

    public:
        std::shared_ptr<Server> get_ptr() { return shared_from_this(); }

        Server(Private, boost::asio::io_context &io_context, const tcp::endpoint &endpoint, bool reuse_addr,
               size_t num_threads);

        static std::shared_ptr<Server> create(boost::asio::io_context &io_context, const tcp::endpoint &endpoint,
                                              bool reuse_addr, size_t num_threads)
        {
            return std::make_shared<Server>(Private(), io_context, endpoint, reuse_addr, num_threads);
        }

        ~Server()
        {
            std::cout << "Server destructor called. Cleaning up resources." << std::endl;
            // Explicit cleanup if needed (for example, stopping io_context)
            acceptor_.close();
            io_context_.stop();
        }

        Server(const Server &) = delete;

        Server &operator=(const Server &) = delete;

        Server(Server &&) noexcept = delete;

        Server &operator=(Server &&) noexcept = delete;

        awaitable<void> listen();

        void start();

        static awaitable<void> start_session(std::shared_ptr<Connection> conn);

    private:
        boost::asio::io_context &io_context_;
        tcp::acceptor acceptor_;
        size_t thread_number_ = 10;
        boost::asio::signal_set signals_;
    };
}  // namespace redis

#endif  // SERVER_HH
