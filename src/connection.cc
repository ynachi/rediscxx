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

    std::string process_data(const std::string& input)
    {
        std::string output = input;
        std::transform(output.begin(), output.end(), output.begin(), ::toupper);
        return output;
    }

    awaitable<void> Connection::process_frames()
    {
        auto const self = this->get_ptr();
        boost::asio::streambuf streambuf;
        for (;;)
        {
            auto n = co_await _socket.async_read_some(boost::asio::buffer(streambuf.prepare(1024)), use_awaitable);
            streambuf.commit(n);  // Commit the read bytes to the buffer

            std::cout << "First read: " << n << " bytes" << std::endl;
            std::cout << "Total bytes after first read: " << streambuf.size() << std::endl;
            if (n != 0)
            {
                // Check if the buffer contains a complete frame/message
                while (true)
                {
                    std::istream is(&streambuf);
                    std::string line;
                    if (std::getline(is, line))
                    {
                        std::string processed_data = process_data(line);
                        std::cout << processed_data << "\n" << std::flush;
                        co_await boost::asio::async_write(_socket, boost::asio::buffer(processed_data), use_awaitable);

                        std::size_t consumed_length = line.length() + 1;  // +1 for the newline character
                        streambuf.consume(consumed_length);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                std::cout << "connection closed by client";
                co_return;
            }
        }
    }
}  // namespace redis
