//
// Created by ynachi on 8/17/24.
//
#include "frame.hh"

namespace redis {
    Frame Frame::make_frame(const FrameID &frame_id) {
        switch (frame_id) {
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

} // namespace redis
