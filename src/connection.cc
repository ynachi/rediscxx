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
        FrameDecoder decoder;
        // while line looks unused, it is here to keep the connection object alive thought the lifetime of this
        // function.
        auto const self = this->get_ptr();
        for (;;)
        {
            auto buffer_space = decoder.get_buffer().prepare(1024);
            const auto n = co_await _socket.async_read_some(buffer_space, use_awaitable);
            decoder.get_buffer().commit(n);  // Commit the read bytes to the buffer
            std::cout << "First read: " << n << " bytes" << std::endl;
            std::cout << "Total bytes after first read: " << decoder.get_buffer().size() << std::endl;
                while (true)
                {
                    if (auto output = decoder.try_get_string(); output.has_value()) {
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
