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
        for (;;)
        {
            auto buffer_space = self->decoder_.get_buffer().prepare(1024);
            const auto n = co_await _socket.async_read_some(buffer_space, use_awaitable);
            self->decoder_.get_buffer().commit(n);  // Commit the read bytes to the buffer
            std::cout << "First read: " << n << " bytes" << std::endl;
            std::cout << "Total bytes after first read: " << self->decoder_.get_buffer().size() << std::endl;
                while (true)
                {
                    if (auto output = this->decoder_.get_simple_string(); output.has_value()) {
                        std::cout << "decoded string: " << output.value() << "\n" << std::flush;
                        co_await async_write(_socket, boost::asio::buffer(output.value() + "\n"), use_awaitable);
                        std::cout << "Done writing back to socket\n";
                    } else {
                        break;
                    }
                }

            if (n == 0) {
                std::cout << "connection closed by client\n";
                co_return;
            }


                // if (is.eof())
                // {
                //     std::cout << "connection closed by client" << std::flush;
                //     streambuf.consume(streambuf.size());
                //     co_return;
                // }
        }
    }
}  // namespace redis
