//
// Created by ynachi on 8/17/24.
//
#include <commands.hh>
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
        std::cout << "before entered the loop: \n";
        boost::asio::dynamic_vector_buffer buffer(self->decoder_.get_buffer());
        for (;;)
        {
            auto buffer_space = buffer.prepare(self->read_chunk_size_);
            const auto [e, n] = co_await _socket.async_read_some(buffer_space, use_nothrow_awaitable);
            if ((e && e == boost::asio::error::eof) || n == 0)
            {
                std::cout << "connection::process_frames: connection closed by user";
                co_return;
            }
            if (e)
            {
                std::cerr << "connection::process_frames: error reading from socket " << e.what();
                continue;
            }
            buffer.commit(n);  // Commit the read bytes to the buffer
            while (true)
            {
                // Get a frame array representing a command
                std::cout << "Calling decode_frame..." << std::endl;
                if (auto output = this->decoder_.decode_frame(); output.has_value())
                {
                    std::cout << "After decode frame, debug" << std::endl;
                    try
                    {
                        std::cout << "Before applying command" << std::endl;
                        std::cout << "received a frame: " << output.value().to_string() << "\n" << std::flush;
                        auto command = Command::command_from_frame(output.value());
                        co_await this->apply_command(command);
                        std::cout << "After applying command" << std::endl;
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Exception in processing frame: " << e.what() << std::endl;
                    }
                }
                else
                {
                    // @TODO: send back an error to the client if needed
                    std::cout << "failed to decode frame \n";
                    break;
                }
            }
        }
    }

    awaitable<void> Connection::apply_command(const Command& command)
    {
        switch (command.type)
        {
            case CommandType::PING:
            {
                std::cout << "connection::apply_command: PING: " << command.args[0] << "\n";
                const Frame response = command.args[0] == "PONG" ? Frame{FrameID::SimpleString, "PONG"}
                                                                 : Frame{FrameID::BulkString, command.args[0]};
                auto [e1, _] =
                        co_await async_write(_socket, boost::asio::buffer(response.to_string()), use_nothrow_awaitable);
                if (e1)
                {
                    std::cerr << "connection::process_frames: error writing to socket";
                }
                std::cout << "Done writing back to socket\n";
                break;
            }
            case CommandType::ERROR:
            {
                std::cout << "connection::apply_command: ERROR: " << command.args[0] << "\n";
                const auto response = Frame{FrameID::BulkString, command.args[0]};
                auto [e1, _] =
                        co_await async_write(_socket, boost::asio::buffer(response.to_string()), use_nothrow_awaitable);
                std::cout << "done connection::apply_command: ERROR: " << command.args[0] << "\n";
                ;
                if (e1)
                {
                    std::cerr << "connection::process_frames: error writing to socket";
                }
                std::cout << "Done writing back to socket\n";
                break;
            }
            default:
                co_return;
        }
    }

}  // namespace redis
