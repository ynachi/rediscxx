//
// Created by ynachi on 8/17/24.
//

#ifndef FRAME_HH
#define FRAME_HH
#include <frame_id.hh>
#include <string>
#include <variant>
#include <vector>

namespace redis
{
    struct Frame
    {
        FrameID frame_id;
        // Vector is used for all aggregate frames.
        // For maps, we double the number of elements.
        // Each k,v is adjacent to the vector.
        // @TODO use std::optional<std::string> to allow to represent null nulk strings
        std::variant<std::monostate, std::string, int64_t, bool, std::vector<Frame>> data;

        static Frame make_frame(const FrameID& frame_id);

        bool operator==(const Frame& other) const { return frame_id == other.frame_id && data == other.data; }

        [[nodiscard]] std::string to_string() const noexcept;
    };
}  // namespace redis

#endif  // FRAME_HH
