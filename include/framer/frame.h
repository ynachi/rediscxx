//
// Created by ynachi on 8/17/24.
//

#ifndef FRAME_HH
#define FRAME_HH
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace redis
{
    using bytes = std::vector<char>;

    constexpr char kInteger = ':';
    constexpr char kSimpleString = '+';
    constexpr char kSimpleError = '-';
    constexpr char kBulkString = '$';
    constexpr char kBulkError = '!';
    constexpr char kBoolean = '#';
    constexpr char kNull = '_';
    constexpr char kBigNumber = '(';
    constexpr char kArray = '*';

    enum class FrameID : char
    {
        Integer = kInteger,  // ':'
        SimpleString = kSimpleString,  // '+'
        SimpleError = kSimpleError,  // '-'
        BulkString = kBulkString,  // '$'
        BulkError = kBulkError,  // '!'
        Boolean = kBoolean,  // '#'
        Null = kNull,  // '_'
        BigNumber = kBigNumber,  // '('
        Array = kArray,  // '*'
        Undefined
    };

    FrameID frame_id_from_char(char from);

    inline bool is_aggregate_frame(const FrameID frame_id) noexcept { return frame_id == FrameID::Array; };
    inline bool is_bulk_frame(const FrameID frame_id) noexcept
    {
        return frame_id == FrameID::BulkString || frame_id == FrameID::BulkError;
    }

    inline bool is_simple_frame(const FrameID frame_id) noexcept
    {
        return frame_id == FrameID::SimpleString || frame_id == FrameID::SimpleError || frame_id == FrameID::BigNumber;
    }
    struct Frame
    {
        FrameID frame_id;
        // Vector is used for all aggregate frames.
        // For maps, we double the number of elements.
        // Each k,v is adjacent to the vector.
        // @TODO use std::optional<std::string> to allow to represent null strings
        std::variant<std::monostate, bytes, int64_t, bool, std::vector<Frame>> data;

        static Frame make_frame(const FrameID& frame_id);

        bool operator==(const Frame& other) const { return frame_id == other.frame_id && data == other.data; }

        [[nodiscard]] std::string to_string() const noexcept;

        [[nodiscard]] std::vector<char> as_bytes() const noexcept;
    };
}  // namespace redis

#endif  // FRAME_HH
