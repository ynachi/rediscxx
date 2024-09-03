#include <protocol.hh>
#include <set>
#include <string>

namespace redis
{

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::_read_simple_string() noexcept
    {
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }

        const size_t size = this->buffer_.size();

        for (size_t i = 0; i < size; ++i)
        {
            switch (this->buffer_[i])
            {
                case '\r':
                    if (i + 1 >= size)
                    {
                        return std::unexpected(FrameDecodeError::Incomplete);
                    }
                    if (this->buffer_[i + 1] != '\n')
                    {
                        // Invalid frame. Throw away up to \r included
                        this->consume(i + 1);
                        return std::unexpected(FrameDecodeError::Invalid);
                    }
                    return std::string(this->buffer_.begin(), this->buffer_.begin() + i);
                case '\n':
                    // Invalid: isolated LF, consume up to the LF included
                    this->consume(i + 1);
                    return std::unexpected(FrameDecodeError::Invalid);
                default:;
            }
        }

        // Incomplete sequence: end of buffer reached without finding CRLF
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::_read_bulk_string(const size_t n) noexcept
    {
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }

        // Do we have enough data to decode the bulk frame?
        if (this->buffer_.size() < n + 2)
        {
            return std::unexpected(FrameDecodeError::Incomplete);
        }

        if (this->buffer_[n] != '\r' || this->buffer_[n + 1] != '\n')
        {
            // remove n+2 bytes which were supposed to be the frame we wanted to decode
            // because they are part of a faulty frame.
            this->consume(n + 2);
            return std::unexpected(FrameDecodeError::Invalid);
        }

        // Let's actually read the data we know is enough
        return std::string(this->buffer_.begin(), this->buffer_.begin() + n);
    }

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::get_simple_string() noexcept
    {
        auto result = this->_read_simple_string();
        if (!result.has_value())
        {
            return std::unexpected(result.error());
        }
        this->consume(result->size() + 2);
        return result;
    }

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::get_bulk_string(const size_t length) noexcept
    {
        auto result = this->_read_bulk_string(length);
        if (!result.has_value())
        {
            return std::unexpected(result.error());
        }
        this->consume(length + 2);
        return result;
    }

    std::expected<int64_t, FrameDecodeError> ProtocolDecoder::get_int() noexcept
    {
        auto str_result = this->get_simple_string();
        if (!str_result.has_value())
        {
            return std::unexpected(str_result.error());
        }
        try
        {
            int64_t result = std::stoll(str_result.value());
            return result;
        }
        catch (const std::exception &_)
        {
            //@TODO log the error
            return std::unexpected(FrameDecodeError::Atoi);
        }
    }

    std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_simple_frame_variant(const FrameID frame_id) noexcept
    {
        if (!std::set{FrameID::SimpleString, FrameID::SimpleError, FrameID::BigNumber}.contains(frame_id))
        {
            return std::unexpected(FrameDecodeError::WrongArgsType);
        }
        auto frame_data = this->get_simple_string();
        if (!frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return frame;
    }

    std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_bulk_frame_variant(const FrameID frame_id) noexcept
    {
        if (!std::set{FrameID::BulkString, FrameID::BulkError}.contains(frame_id))
        {
            return std::unexpected(FrameDecodeError::WrongArgsType);
        }
        auto size = this->get_int();
        if (!size.has_value())
        {
            return std::unexpected(size.error());
        }
        auto frame_data = this->get_bulk_string(size.value());
        if (!frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return frame;
    }

    std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_integer_frame() noexcept
    {
        auto frame_data = this->get_int();
        if (!frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(FrameID::Integer);
        frame.data = frame_data.value();
        return frame;
    }

    std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_boolean_frame() noexcept
    {
        auto frame_data = this->get_simple_string();
        if (!frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        if (frame_data.value() != "t" && frame_data.value() != "f")
        {
            return std::unexpected(FrameDecodeError::Invalid);
        }
        Frame frame = Frame::make_frame(FrameID::Boolean);
        frame.data = (frame_data.value() == "t");
        return frame;
    }

    std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_null_frame() noexcept
    {
        if (auto frame_data = this->get_simple_string(); !frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        return Frame::make_frame(FrameID::Null);
    }

}  // namespace redis
