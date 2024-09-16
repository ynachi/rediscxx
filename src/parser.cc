#include <iostream>
#include <parser.hh>
#include <set>
#include <stack>
#include <string>

namespace redis
{
    std::ostream &operator<<(std::ostream &os, const ProtocolParser &parser)
    {
        os << std::format("Parser Buffer - Size={}, Content={}", parser.buffer_.size(),
                          std::string(parser.buffer_.begin(), parser.buffer_.end()));
        return os;
    }

    std::expected<std::string, FrameDecodeError> ProtocolParser::get_simple_string() noexcept
    {
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }
        const size_t size = this->buffer_.size();
        const auto prev_cursor_position = this->buffer_.cbegin() + static_cast<int>(cursor_);

        for (size_t i = this->cursor_; i < size; ++i)
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
                        this->set_cursor(i + 1);
                        return std::unexpected(FrameDecodeError::Invalid);
                    }
                    // success, move the cursor after CRLF. As we are at CR, we need to move 2 steps.
                    this->set_cursor(i + 2);
                    // Don't include CRLF so the left boundary should be CR non-inclusive
                    return std::string(prev_cursor_position, this->buffer_.cbegin() + static_cast<int>(i));
                case '\n':
                    // Invalid: isolated LF, consume up to the LF included
                    this->set_cursor(i + 1);
                    return std::unexpected(FrameDecodeError::Invalid);
                default:;
            }
        }

        // Incomplete sequence: end of buffer reached without finding CRLF
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    std::expected<std::string, FrameDecodeError> ProtocolParser::get_bulk_string(const size_t length) noexcept
    {
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }

        // Do we have enough data to decode the bulk frame?
        if (this->buffer_.size() - cursor_ < length + 2)
        {
            return std::unexpected(FrameDecodeError::Incomplete);
        }

        // save the startup position
        const auto left = this->buffer_.cbegin() + static_cast<int>(cursor_);

        if (this->buffer_[length + cursor_] != '\r' || this->buffer_[length + cursor_ + 1] != '\n')
        {
            // move the cursor after what is supposed to be the end of the frame
            // we need to move to the next char to process
            this->set_cursor(cursor_ + length + 2);
            return std::unexpected(FrameDecodeError::Invalid);
        }

        // Let's actually read the data we know is enough
        this->set_cursor(cursor_ + length + 2);
        return std::string(left, left + static_cast<int>(length));
    }

    std::expected<int64_t, FrameDecodeError> ProtocolParser::get_int() noexcept
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

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_simple_frame_variant(const FrameID frame_id) noexcept
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

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_bulk_frame_variant(const FrameID frame_id) noexcept
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

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_integer_frame() noexcept
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

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_boolean_frame() noexcept
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

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_null_frame() noexcept
    {
        if (auto frame_data = this->get_simple_string(); !frame_data.has_value())
        {
            return std::unexpected(frame_data.error());
        }
        return Frame::make_frame(FrameID::Null);
    }

    std::expected<Frame, FrameDecodeError> ProtocolParser::get_array_frame() noexcept
    {
        return this->_get_aggregate_frame(FrameID::Array);
    }

    FrameID ProtocolParser::get_frame_id()
    {
        assert(!this->buffer_.empty());
        const auto frame_token = *(buffer_.begin() + static_cast<int>(cursor_));
        this->set_cursor(this->cursor_ + 1);
        return frame_id_from_u8(frame_token);
    }


    std::expected<Frame, FrameDecodeError> ProtocolParser::decode_frame() noexcept
    {
        std::cout << *this << "\n";
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }

        std::expected<Frame, FrameDecodeError> result;
        switch (const auto frame_id = this->get_frame_id())
        {
            case FrameID::SimpleString:
            case FrameID::SimpleError:
            case FrameID::BigNumber:
                result = this->get_simple_frame_variant(frame_id);
                break;
            case FrameID::BulkString:
            case FrameID::BulkError:
                result = this->get_bulk_frame_variant(frame_id);
                break;
            case FrameID::Integer:
                result = this->get_integer_frame();
                break;
            case FrameID::Boolean:
                result = this->get_boolean_frame();
                break;
            case FrameID::Null:
                result = this->get_null_frame();
                break;
            case FrameID::Array:
                result = this->get_array_frame();
                break;
            default:
                result = std::unexpected(FrameDecodeError::UndefinedFrame);
        }
        // Do not consume in case of incomplete frame.
        // In all other cases, consume the processed data.
        if (result.has_value() || result.error() != FrameDecodeError::Incomplete)
        {
            this->consume();
        }
        else if (result.error() == FrameDecodeError::Incomplete)
        {
            // bring back the cursor to the beginning of the stream as we try to decode again from the beginning.
            // For this reason, the user should evaluate the sizes of most frames and set the buffer size accordingly.
            // Because it can be costly to don't decode a full frame in one shot.
            this->set_cursor(0);
        }
        return result;
    }

    std::expected<Frame, FrameDecodeError> ProtocolParser::_get_aggregate_frame(FrameID frame_type) noexcept
    {
        // "3\r\n:1\r\n:2\r\n:3\r\n" -> [1, 2, 3]
        // "*2\r\n:1\r\n*1\r\n+Three\r\n&&" -> [1, ["Tree"]]

        // @TODO: double size if frame type is map
        const auto maybe_size = this->get_int();
        if (!maybe_size.has_value())
        {
            return std::unexpected(maybe_size.error());
        }

        auto size = maybe_size.value();
        std::vector<Frame> frames;
        frames.reserve(size);
        // we keep track of
        // 1. the type of aggregate frame we need to process
        // 2. the number inner frames left to decode
        // 3. The container where we store the frames for the current aggregate
        std::stack<std::tuple<FrameID, int64_t, std::vector<Frame>>> stack;
        stack.emplace(frame_type, size, frames);

        while (!stack.empty())
        {
            // get the next frame type
            if (!this->has_data())
            {
                // we expect more data as we are not done yet.
                return std::unexpected(FrameDecodeError::Incomplete);
            }

            auto next_frame_id = this->get_frame_id();

            // is the next frame also aggregate?
            if (is_aggregate_frame(next_frame_id))
            {
                const auto maybe_next_size = this->get_int();
                if (!maybe_next_size.has_value())
                {
                    return std::unexpected(maybe_next_size.error());
                }
                stack.emplace(next_frame_id, maybe_next_size.value(), std::vector<Frame>{});
                continue;
            }

            auto next_frame = this->_get_non_aggregate_frame(next_frame_id);
            if (!next_frame.has_value())
            {
                return std::unexpected(next_frame.error());
            }
            // append the frame in place
            auto &latest_tuple = stack.top();
            std::get<2>(latest_tuple).push_back(std::move(next_frame.value()));
            --std::get<1>(latest_tuple);

            // If count == 0, we've decoded an entire array.
            // So push it to the penultimate
            // aggregate in the stack if any.
            // If there's no more array in the stack, this means
            // we should return as the total frame was completely processed.
            if (std::get<1>(latest_tuple) == 0)
            {
                while (true)
                {
                    // Warning here! we need to copy or move as using the reference would let the value
                    // in an undefined state.
                    auto parent_tuple = std::move(stack.top());
                    stack.pop();
                    // The full global frame was decoded, so return it
                    if (stack.empty())
                    {
                        // Here is why we needed to keep track of the IDs,
                        // to build the right aggregate.
                        Frame final_result = Frame::make_frame(std::get<0>(parent_tuple));
                        final_result.data = std::move(std::get<2>(parent_tuple));
                        return final_result;
                    }
                    // we fully decoded an intermediate aggregate but not the full global frame
                    auto &child_tuple = stack.top();
                    Frame child_aggregate_frame = Frame::make_frame(std::get<0>(child_tuple));
                    child_aggregate_frame.data = std::move(std::get<2>(parent_tuple));
                    std::get<2>(child_tuple).push_back(std::move(child_aggregate_frame));
                    --std::get<1>(child_tuple);
                    // 0 means there are no longer frame to process for the child, so we should continue to
                    // pop and push already processed one. Anything different to 0 means we have to continue to
                    // process new frames. This is why we need to exit the loop in this case.
                    if (std::get<1>(child_tuple) != 0)
                    {
                        break;
                    }
                }
            }
        }
        return std::unexpected(FrameDecodeError::Invalid);
    }

    std::expected<Frame, FrameDecodeError> ProtocolParser::_get_non_aggregate_frame(FrameID frame_id) noexcept
    {
        assert(!is_aggregate_frame(frame_id));
        std::expected<Frame, FrameDecodeError> result;
        if (is_bulk_frame(frame_id))
        {
            result = this->get_bulk_frame_variant(frame_id);
        }
        else if (is_simple_frame(frame_id))
        {
            result = this->get_simple_frame_variant(frame_id);
        }
        else if (frame_id == FrameID::Integer)
        {
            result = this->get_integer_frame();
        }
        else if (frame_id == FrameID::Boolean)
        {
            result = this->get_boolean_frame();
        }
        else
        {
            result = this->get_null_frame();
        }
        return result;
    }
}  // namespace redis
