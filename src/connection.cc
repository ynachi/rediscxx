//
// Created by ynachi on 8/17/24.
//
#include <connection.hh>
#include <iostream>

namespace redis
{
    using boost::asio::awaitable;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    awaitable<void> Connection::process_frames()
    {
        auto const self = this->get_ptr();
        std::array<char, 1024> data{};
        for (;;)
        {
            auto n = co_await _socket.async_read_some(boost::asio::buffer(data), use_awaitable);
            if (n != 0)
            {
                // Process the data: convert to uppercase
                std::transform(data.begin(), data.begin() + n, data.begin(),
                               [](unsigned char c) { return std::toupper(c); });
                std::cout << data.data() << "\n" << std::flush;
                co_await boost::asio::async_write(_socket, boost::asio::buffer(data, n), use_awaitable);
            }
            else
            {
                std::cout << "connection closed by client";
                co_return;
            }
        }
    }
}  // namespace redis
