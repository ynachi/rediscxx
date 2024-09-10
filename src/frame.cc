//
// Created by ynachi on 8/17/24.
//
#include <format>
#include <frame.hh>
#include <sstream>

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
            case FrameID::BulkString:
            case FrameID::BulkError:
                return Frame(frame_id, std::string{});
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

    // @TODO: make this function iterative
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
            case FrameID::Boolean:
                return std::get<bool>(this->data) ? "#t\r\n" : "#f\r\n";
            case FrameID::Array:
            {
                std::stringstream ss;
                const auto &frame_value = std::get<std::vector<Frame>>(this->data);
                ss << '*' << frame_value.size();
                for (const auto &item: frame_value)
                {
                    ss << item.to_string();
                }
                return ss.str();
            }
            case FrameID::Null:
            default:
                return "_\r\n";
        }
    }

}  // namespace redis
