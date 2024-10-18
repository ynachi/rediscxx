//
// Created by ynachi on 10/6/24.
//

#include "framer/handler.h"

#include <photon/common/alog.h>


namespace redis
{
    constexpr char CR = '\r';
    constexpr char LF = '\n';

    //@TODO add a seastar logger
    //@TODO add an opentelemetry span

    Handler::Handler(photon::net::ISocketStream* stream, const size_t chunk_size) :
        chunck_size_(chunk_size), stream_(stream)
    {
        buffer_.reserve(chunck_size_);
    }

    //@TODO: maybe just return the number of bytes read and let the caller unpack the error
    ssize_t Handler::read_more_from_stream()
    {
        if (const auto buf_current_capacity = this->buffer_.capacity();
            buf_current_capacity - this->buffer_size() < this->chunck_size_)
        {
            this->buffer_.reserve(this->chunck_size_ + buf_current_capacity);
        }
        auto num_read = this->stream_->recv(this->buffer_.data(), this->chunck_size_);
        if (num_read < 0)
        {
            LOG_DEBUG("fatal network error occurred");
            // return std::unexpected(DecodeError::FatalNetworkError);
        }
        if (num_read == 0)
        {
            LOG_DEBUG("client orderly closed connection");
            // return std::unexpected(DecodeError::Eof);
        }
        LOG_DEBUG("read ", num_read, " number of bytes from the network");
        return 0;
    }


    // void Handler::append_data(const seastar::temporary_buffer<char>&& data)
    // {
    //     buffer_.insert(buffer_.end(), data.begin(), data.end());
    // }


    std::expected<std::string, Handler::DecodeError> Handler::read_simple_string_()
    {
        if (this->buffer_.size() < 2)
        {
            // not enough data to decode a frame
            return std::unexpected(DecodeError::Incomplete);
        }

        const auto current_cursor = static_cast<int>(this->cursor_);
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
                    this->cursor_ += i + 1;
                    return std::unexpected(DecodeError::Invalid);
                }
                auto result = std::string(this->buffer_.begin() + current_cursor,
                                          this->buffer_.begin() + current_cursor + i + 1);
                this->cursor_ += i + 2;
                return result;
            }
            if (current_char == LF)
            {
                // this frame is in valid because LF is the starting of it, of single CR. e.g: hel\rlo
                this->cursor_ += i + 1;
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
        catch (const std::exception& _)
        {
            //@TODO log the error
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
        const auto left = this->buffer_.cbegin() + static_cast<int>(cursor_);

        if (this->buffer_[size + cursor_] != '\r' || this->buffer_[size + cursor_ + 1] != '\n')
        {
            // move the cursor after what is supposed to be the end of the frame
            // we need to move to the next char to process
            this->cursor_ += size + 2;
            return std::unexpected(DecodeError::Invalid);
        }

        // Let's actually read the data we know is enough
        this->cursor_ += size + 2;
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
