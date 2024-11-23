//
// Created by ynachi on 8/17/24.
//

#ifndef FRAME_ID_HH
#define FRAME_ID_HH
#include <cstdint>

namespace redis
{
    constexpr uint8_t kInteger = 58;  // ':'
    constexpr uint8_t kSimpleString = 43;  // '+'
    constexpr uint8_t kSimpleError = 45;  // '-'
    constexpr uint8_t kBulkString = 36;  // '$'
    constexpr uint8_t kBulkError = 33;  // '!'
    constexpr uint8_t kBoolean = 35;  // '#'
    constexpr uint8_t kNull = 95;  // '_'
    constexpr uint8_t kBigNumber = 40;  // '('
    constexpr uint8_t kArray = 42;  // '*'

    enum class FrameID : uint8_t
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

    FrameID frame_id_from_u8(uint8_t from);

    inline bool is_aggregate_frame(const FrameID frame_id) noexcept { return frame_id == FrameID::Array; };
    inline bool is_bulk_frame(const FrameID frame_id) noexcept
    {
        return frame_id == FrameID::BulkString || frame_id == FrameID::BulkError;
    }

    inline bool is_simple_frame(const FrameID frame_id) noexcept
    {
        return frame_id == FrameID::SimpleString || frame_id == FrameID::SimpleError || frame_id == FrameID::BigNumber;
    }
}  // namespace redis

#endif  // FRAME_ID_HH
