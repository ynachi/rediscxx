//
// Created by ynachi on 10/6/24.
//

#include "framer/handler.h"

#include <utility>


namespace redis
{
    constexpr char CR = '\r';
    constexpr char LF = '\n';

    //@TODO add an open telemetry span
    Handler::Handler(seastar::connected_socket&& stream, const seastar::socket_address& addr,
                     seastar::lw_shared_ptr<seastar::logger> logger, const size_t chunk_size) :
        chunck_size_(chunk_size), connected_socket_(std::move(stream)),
        // @TODO param me
        input_stream_(connected_socket_.input(seastar::connected_socket_input_stream_config(chunck_size_, 512, 8192))),
        // @TODO param me
        output_stream_(connected_socket_.output(8192)), remote_address_(addr), logger_(logger)
    {
        logger->debug("new connection from {}", addr);
    }

    seastar::future<std::expected<void, Handler::DecodeError>> Handler::read_more_data()
    {
        auto tmp_buffer = co_await this->input_stream_.read();
        if (tmp_buffer.empty())
        {
            co_return std::unexpected(DecodeError::Eof);
        }
    }


    std::expected<std::string, Handler::DecodeError> Handler::read_simple_string_()
    {
        if (this->buffer_.size() < 2)
        {
            // not enough data to decode a frame
            return std::unexpected(DecodeError::Incomplete);
        }

        const auto current_cursor = static_cast<int>(this->read_cursor_);
        for (auto i = current_cursor; i < this->buffer_.size() - 1; ++i)
        {
            const auto current_char = this->buffer_[i];
            if (current_char == CR)
            {
                if (i + 1 < this->buffer_.size())
                {
                    // This is an incomplete frame, maybe. e.g: hello\r
                    return std::unexpected(DecodeError::Incomplete);
                }

                if (this->buffer_[i + 1] != LF)
                {
                    // this frame is in valid because LF is the starting of it, of single CR. e.g: hel\rlo
                    this->read_cursor_ += i + 1;
                    return std::unexpected(DecodeError::Invalid);
                }
                auto result = std::string(this->buffer_.begin() + current_cursor,
                                          this->buffer_.begin() + current_cursor + i + 1);
                this->read_cursor_ += i + 2;
                return result;
            }
            if (current_char == LF)
            {
                // this frame is in valid because LF is the starting of it, of single CR. e.g: hel\rlo
                this->read_cursor_ += i + 1;
                return std::unexpected(DecodeError::Invalid);
            }
        }
        // If no CR is found till the end, it means the frame is incomplete
        return std::unexpected(DecodeError::Incomplete);
    }

    std::expected<int64_t, Handler::DecodeError> Handler::read_integer_()
    {
        auto result = this->read_simple_string_();
        if (!result.has_value())
        {
            return std::unexpected(result.error());
        }

        try
        {
            return std::stoll(result.value());
        }
        catch (const std::exception& e)
        {
            logger_->debug("exception in parsing integer: {}", e.what());
            return std::unexpected(DecodeError::Atoi);
        }
    }

    std::expected<std::string, Handler::DecodeError> Handler::read_bulk_string_()
    {
        auto size_result = this->read_integer_();
        if (!size_result.has_value())
        {
            return std::unexpected(size_result.error());
        }

        const auto size = size_result.value();

        // save the startup position
        const auto left = this->buffer_.cbegin() + static_cast<int>(read_cursor_);

        if (this->buffer_[size + read_cursor_] != '\r' || this->buffer_[size + read_cursor_ + 1] != '\n')
        {
            // move the cursor after what is supposed to be the end of the frame
            // we need to move to the next char to process
            this->read_cursor_ += size + 2;
            return std::unexpected(DecodeError::Invalid);
        }

        // Let's actually read the data we know is enough
        this->read_cursor_ += size + 2;
        return std::string(left, left + static_cast<int>(size));
    }

    std::expected<Frame, Handler::DecodeError> Handler::read_simple_frame()
    {
        auto result = this->read_simple_string_();
        if (!result.has_value())
        {
            return std::unexpected(result.error());
        }
        return Frame{FrameID::SimpleString, result.value()};
    }

}  // namespace redis
