//
// Created by ynachi on 8/17/24.
//
#include <format>
#include <framer/frame.h>
#include <iostream>
#include <sstream>

namespace redis
{
    FrameID frame_id_from_char(const char from)
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

    // use this for debug
    std::string Frame::to_string() const noexcept
    {
        auto data = this->as_bytes();
        return std::string(data.begin(), data.end());
    }

    bytes Frame::as_bytes() const noexcept
    {
        std::vector<char> out;
        out.push_back(static_cast<char>(frame_id));
        switch (this->frame_id)
        {
            case FrameID::Integer:
            {
                auto data_str = std::to_string(std::get<int64_t>(this->data));
                out.insert(out.end(), data_str.begin(), data_str.end());
                out.push_back('\r');
                out.push_back('\n');
                break;
            }
            case FrameID::SimpleString:
            case FrameID::SimpleError:
            case FrameID::BigNumber:
            {
                out.insert(out.end(), std::get<bytes>(this->data).begin(), std::get<bytes>(this->data).end());
                out.push_back('\r');
                out.push_back('\n');
                break;
            }
            case FrameID::BulkString:
            case FrameID::BulkError:
            {
                auto data = std::get<bytes>(this->data);
                auto size_str = std::to_string(data.size());
                out.insert(out.end(), size_str.begin(), size_str.end());
                out.push_back('\r');
                out.push_back('\n');
                out.insert(out.end(), data.begin(), data.end());
                out.push_back('\r');
                out.push_back('\n');
                break;
            }
            case FrameID::Boolean:
            {
                auto content = std::get<bool>(this->data) ? std::vector{'t', '\r', '\n'} : std::vector{'f', '\r', '\n'};
                out.insert(out.end(), content.begin(), content.end());
                break;
            }
            case FrameID::Array:
            {
                for (const auto &frame_value = std::get<std::vector<Frame>>(this->data); const auto &item: frame_value)
                {
                    out.insert(out.end(), item.as_bytes().begin(), item.as_bytes().end());
                }
                break;
            }
            case FrameID::Null:
            default:
            {
                out.push_back('\r');
                out.push_back('\n');
                break;
            }
        }
        return out;
    }

}  // namespace redis
