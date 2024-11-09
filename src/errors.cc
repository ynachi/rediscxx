#include "errors.hh"

#include <iostream>

namespace redis {
    std::ostream &operator<<(std::ostream &o, RedisError err) {
        switch (err) {
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
        }
    }
} // namespace redis
