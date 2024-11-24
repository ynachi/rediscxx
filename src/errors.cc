#include "errors.h"

#include <iostream>

namespace redis
{
    std::ostream &operator<<(std::ostream &o, const RedisError err)
    {
        switch (err)
        {
            case RedisError::success:
                return o << "RedisError::success";
            case RedisError::invalid_frame:
                return o << "RedisError::invalid_frame";
            case RedisError::incomplete_frame:
                return o << "RedisError::incomplete_frame";
            case RedisError::empty_buffer:
                return o << "RedisError::empty_buffer";
            case RedisError::atoi:
                return o << "RedisError::atoi";
            case RedisError::wrong_arg_types:
                return o << "RedisError::wrong_arg_types";
            case RedisError::eof:
                return o << "RedisError::eof";
            case RedisError::not_enough_data:
                return o << "RedisError::not_enough_data";
            //@TODO categorize
            case RedisError::generic_network_error:
                return o << "RedisError::generic_network_error";
        }
        return o << "redis::RedisError::unknown";
    }
}  // namespace redis
