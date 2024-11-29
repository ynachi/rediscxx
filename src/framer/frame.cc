//
// Created by ynachi on 8/17/24.
//
#include <format>
#include <framer/frame.h>
#include <iostream>
#include <sstream>

namespace redis
{
    FrameID frame_id_from_u8(const uint8_t from)
    {
        switch (from)
        {
            using enum FrameID;
            case kInteger:
                return Integer;
            case kSimpleString:
                return SimpleString;
            case kSimpleError:
                return SimpleError;
            case kBulkString:
                return BulkString;
            case kBulkError:
                return BulkError;
            case kBoolean:
                return Boolean;
            case kNull:
                return Null;
            case kBigNumber:
                return BigNumber;
            case kArray:
                return Array;
            default:
                return Undefined;
        }
    }

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
                return Frame(frame_id, bytes{});
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
    // std::string Frame::to_string() const noexcept
    // {
    //     switch (this->frame_id)
    //     {
    //         case FrameID::Integer:
    //             return std::format(":{}\r\n", std::get<int64_t>(this->data));
    //         case FrameID::SimpleString:
    //         case FrameID::SimpleError:
    //         case FrameID::BigNumber:
    //             std::cout << "Debug:to_string - simple" << '\n';
    //             return std::format("{}{}\r\n", static_cast<char>(frame_id), std::get<bytes>(this->data));
    //         case FrameID::BulkString:
    //         case FrameID::BulkError:
    //             std::cout << "Debug:to_string - bulk something" << '\n';
    //             return std::format("{}{}\r\n{}\r\n", static_cast<char>(frame_id),
    //                                std::get<std::string>(this->data).size(), std::get<std::string>(this->data));
    //         case FrameID::Boolean:
    //             std::cout << "Debug:to_string - bool" << '\n';
    //             return std::get<bool>(this->data) ? "#t\r\n" : "#f\r\n";
    //         case FrameID::Array:
    //         {
    //             std::stringstream ss;
    //             const auto &frame_value = std::get<std::vector<Frame>>(this->data);
    //             ss << '*' << frame_value.size() << "\r\n";
    //             for (const auto &item: frame_value)
    //             {
    //                 ss << item.to_string();
    //             }
    //             std::cout << "Debug:to_string" << ss.str() << '\n';
    //             return ss.str();
    //         }
    //         case FrameID::Null:
    //         default:
    //             return "_\r\n";
    //     }
    // }

}  // namespace redis
