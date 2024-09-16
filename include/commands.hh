//
// Created by ynachi on 9/12/24.
//

#ifndef COMMAND_HH
#define COMMAND_HH

#include <frame.hh>
#include <string>
#include <unordered_map>
#include <vector>
namespace redis
{
    enum class CommandType
    {
        PING,
        GET,
        SET,
        DEL,
        EXPIRE,
        ERROR  // This isn't a command per se. But it is used to send erroneous responses back to the user.
    };

    struct Command
    {
        CommandType type;
        std::vector<std::string> args;

        Command(const CommandType t, std::vector<std::string> args) : type(t), args(std::move(args)) {}

        static Command command_from_frame(const Frame& frame) noexcept;
    };

    static std::unordered_map<std::string, CommandType> redis_command_map = {
            {"PING", CommandType::PING}, {"GET", CommandType::GET},       {"SET", CommandType::SET},
            {"DEL", CommandType::DEL},   {"EXPIRE", CommandType::EXPIRE},
    };


}  // namespace redis

#endif  // COMMAND_HH
