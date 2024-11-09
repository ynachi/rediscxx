//
// Created by ynachi on 8/17/24.
//

#ifndef FRAME_HH
#define FRAME_HH
#include "frame_id.hh"
#include <string>
#include <variant>
#include <vector>

namespace redis {
    struct Frame {
        FrameID frame_id;
        std::variant<std::monostate, std::string, int64_t, bool, std::pair<size_t, std::string>, std::vector<Frame>>
                data;

        // prevent to directly instantiate frames
        // Frame() = delete;

        static Frame make_frame(const FrameID &frame_id);
    };
} // namespace redis

#endif // FRAME_HH
