//
// Created by ynachi on 8/17/24.
//
#include "frame_id.hh"

namespace redis {
    std::optional<FrameID> frame_id_from_u8(const uint8_t from) {
        switch (from) {
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
                return std::nullopt;
        }
    }
} // namespace redis
