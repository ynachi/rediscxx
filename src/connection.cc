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
        std::ranges::transform(output, output.begin(), ::toupper);
        return output;
    }

    awaitable<void> Connection::process_frames()
    {
        auto const self = this->get_ptr();
        boost::asio::streambuf streambuf;
        for (;;)
        {
            const auto n = co_await _socket.async_read_some(buffer(streambuf.prepare(1024)), use_awaitable);
            streambuf.commit(n);  // Commit the read bytes to the buffer
            std::cout << "First read: " << n << " bytes" << std::endl;
            std::cout << "Total bytes after first read: " << streambuf.size() << std::endl;
                std::istream is(&streambuf);
                std::string line;
                while (std::getline(is, line))
                {
                    std::string processed_data = process_data(line);
                    std::cout << processed_data << "\n" << std::flush;
                    co_await async_write(_socket, boost::asio::buffer(processed_data + "\n"), use_awaitable);
                }

                if (is.eof())
                {
                    std::cout << "connection closed by client" << std::flush;
                    streambuf.consume(streambuf.size());
                    co_return;
                }
        }
    }
}  // namespace redis
