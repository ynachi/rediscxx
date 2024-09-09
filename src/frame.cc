//
// Created by ynachi on 8/17/24.
//
#include <format>
#include <frame.hh>

namespace redis
{
    Frame Frame::make_frame(const FrameID &frame_id)
    {
        switch (frame_id)
        {
            case FrameID::Integer:
                return Frame(frame_id, int64_t{0});
            case FrameID::SimpleString:
            case FrameID::SimpleError:
            case FrameID::BigNumber:
                return Frame(frame_id, std::string{});
            case FrameID::BulkString:
            case FrameID::BulkError:
                return Frame(frame_id, std::pair{size_t{0}, std::string{}});
            case FrameID::Boolean:
                return Frame(frame_id, false);
            case FrameID::Null:
                return Frame(frame_id, std::monostate());
            case FrameID::Array:
                return Frame(frame_id, std::vector<Frame>{});
        }
        // We should normally not reach this line
        return Frame{FrameID::Undefined, std::monostate()};
    }

    std::string Frame::to_string() const noexcept
    {
        switch (this->frame_id)
        {
            case FrameID::Integer:
                return std::format(":{}\r\n", std::get<int64_t>(this->data));
            case FrameID::SimpleString:
            case FrameID::SimpleError:
            case FrameID::BigNumber:
                return std::format("{}{}\r\n", static_cast<char>(frame_id), std::get<std::string>(this->data));
            case FrameID::BulkString:
            case FrameID::BulkError:
                return std::format("{}{}\r\n{}\r\n", static_cast<char>(frame_id),
                                   std::get<std::string>(this->data).size(), std::get<std::string>(this->data));
            default:
                return {};
        }
    }

}  // namespace redis
