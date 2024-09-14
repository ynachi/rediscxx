//
// Created by ynachi on 9/12/24.
//

#include <commands.hh>
#include <format>
#include <iostream>
#include <optional>
#include <strings.hh>

namespace redis
{
    std::optional<Command> _check_array(const Frame& frame)
    {
        if (frame.frame_id != FrameID::Array)
        {
            return Command{CommandType::ERROR, {"only array can represent a redis command"}};
        }

        const auto& array_content = std::get<std::vector<Frame>>(frame.data);

        if (array_content.empty())
        {
            return Command{CommandType::ERROR, {"cannot parse command from empty frame array"}};
        }

        for (const auto& [frame_id, _]: array_content)
        {
            if (frame_id != FrameID::BulkString)
            {
                return Command{CommandType::ERROR, {"redis command should be an array of bulk strings only"}};
            }
        }
        return std::nullopt;
    }

    std::string _get_string_at_index(const std::vector<Frame>& frames, const int index)
    {
        const auto& frame_content = std::get<std::string>(frames.at(index).data);
        return frame_content;
    }

    Command _parse_ping_command(const std::vector<Frame>& frames)
    {
        if (frames.size() > 2)
        {
            return Command{CommandType::ERROR, {"PING command must have at most 1 argument"}};
        }
        Command ping_cmd{CommandType::PING, {"PONG"}};
        if (frames.size() == 2)
        {
            auto ping_msg = _get_string_at_index(frames, 1);
            ping_cmd.args[0] = std::move(ping_msg);
        }
        return ping_cmd;
    }

    Command Command::command_from_frame(const Frame& frame) noexcept
    {
        std::cout << "received a frame: " << frame.to_string() << "\n";
        // check the validity of the frame first
        if (const auto& frame_status = _check_array(frame); frame_status.has_value())
        {
            return frame_status.value();
        }

        // get the command name
        const auto& array_content = std::get<std::vector<Frame>>(frame.data);
        const auto& command_name = _get_string_at_index(array_content, 0);

        // is it an existing known command?
        if (!redis_command_map.contains(utils::to_upper(command_name)))
        {
            return Command{CommandType::ERROR, {std::format("unknown command '{}'", command_name)}};
        }

        switch (const auto command = redis_command_map.at(utils::to_upper(command_name)); command)
        {
            case CommandType::PING:
                return _parse_ping_command(array_content);
            default:
                return Command{CommandType::ERROR, {std::format("unknown command '{}'", command_name)}};
        }
    }

}  // namespace redis
