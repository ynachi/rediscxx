//
// Created by ynachi on 10/6/24.
//

#include "framer/handler.h"

#include <photon/common/alog.h>


namespace redis
{
    constexpr char CR = '\r';
    constexpr char LF = '\n';

    Handler::Handler(photon::net::ISocketStream* stream, const size_t chunk_size) :
        chunk_size_(chunk_size), stream_(stream)
    {
        buffer_.reserve(chunk_size_ * 2);
    }

    //@TODO: maybe just return the number of bytes read and let the caller unpack the error
    ssize_t Handler::read_more_from_stream()
    {
        char buf[this->chunk_size_];
        auto num_read = this->stream_->recv(buf, this->chunk_size_);
        if (num_read < 0)
        {
            LOG_DEBUG("fatal network error occurred");
            return num_read;
        }
        if (num_read == 0)
        {
            LOG_DEBUG("nothing to read from the network, probably reach EOF");
            return 0;
        }
        LOG_DEBUG("read ", num_read, " number of bytes from the network");
        this->buffer_.insert(this->buffer_.end(), buf, buf + num_read);
        return num_read;
    }


    std::expected<std::string, Handler::DecodeError> Handler::read_simple_string_()
    {
        if (this->buffer_.size() < 2)
        {
            // not enough data to decode a frame
            return std::unexpected(DecodeError::Incomplete);
        }

        const auto current_cursor = static_cast<int>(this->cursor_pos_);
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
                    this->cursor_pos_ += i + 1;
                    return std::unexpected(DecodeError::Invalid);
                }
                auto result = std::string(this->buffer_.begin() + current_cursor,
                                          this->buffer_.begin() + current_cursor + i + 1);
                this->cursor_pos_ += i + 2;
                return result;
            }
            if (current_char == LF)
            {
                // this frame is in valid because LF is the starting of it, of single CR. e.g: hel\rlo
                this->cursor_pos_ += i + 1;
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
        const auto left = this->buffer_.cbegin() + static_cast<int>(cursor_pos_);

        if (this->buffer_[size + cursor_pos_] != '\r' || this->buffer_[size + cursor_pos_ + 1] != '\n')
        {
            // move the cursor after what is supposed to be the end of the frame
            // we need to move to the next char to process
            this->cursor_pos_ += size + 2;
            return std::unexpected(DecodeError::Invalid);
        }

        // Let's actually read the data we know is enough
        this->cursor_pos_ += size + 2;
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
