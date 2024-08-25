//
// Created by ynachi on 8/17/24.
//

#ifndef CONNECTION_HH
#define CONNECTION_HH

#include <boost/asio.hpp>

#include "protocol.hh"

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    class Connection : public std::enable_shared_from_this<Connection>
    {
        struct Private
        {
            explicit Private() = default;
        };

    public:
        awaitable<void> process_frames();

        std::shared_ptr<Connection> get_ptr() { return shared_from_this(); }

        static std::shared_ptr<Connection> create(tcp::socket socket)
        {
            return std::make_shared<Connection>(Private(), std::move(socket));
        }

        Connection(Private, tcp::socket socket) noexcept : _socket(std::move(socket)), _buffer() {}

        // Delete all other constructors to enforce factory method usage
        Connection(const Connection &) = delete;

        Connection(Connection &&) = delete;

        Connection &operator=(const Connection &) = delete;

        Connection &operator=(Connection &&) = delete;

    private:
        tcp::socket _socket;
        BufferManager _buffer;
    };
}  // namespace redis


#endif  // CONNECTION_HH
