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

    constexpr auto use_nothrow_awaitable = as_tuple(use_awaitable);

    awaitable<void> Connection::process_frames()
    {
        auto const self = this->get_ptr();
        for (;;)
        {
            auto buffer_space = self->decoder_.get_buffer().prepare(1024);
            const auto [e, n] = co_await _socket.async_read_some(buffer_space, use_nothrow_awaitable);
            if ((e && e == boost::asio::error::eof) || n == 0)
            {
                std::cout << "connection::process_frames: connection closed by user";
                co_return;
            }
            if (e)
            {
                std::cout << "connection::process_frames: error reading from socket " << e.what();
                continue;
            }
            self->decoder_.get_buffer().commit(n);  // Commit the read bytes to the buffer
            std::cout << "First read: " << n << " bytes" << std::endl;
            std::cout << "Total bytes after first read: " << self->decoder_.get_buffer().size() << std::endl;
                while (true)
                {
                    if (auto output = this->decoder_.get_simple_string(); output.has_value()) {
                        std::cout << "decoded string: " << output.value() << "\n" << std::flush;
                        auto [e1, _] = co_await async_write(_socket, boost::asio::buffer(output.value() + "\n"), use_nothrow_awaitable);
                        if (e1)
                        {
                            std::cerr << "connection::process_frames: error writing to socket";
                        }
                        std::cout << "Done writing back to socket\n";
                    } else {
                        break;
                    }
                }
        }
    }
}  // namespace redis
