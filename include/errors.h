#pragma once

#include "framer/frame.h"

#include <system_error>
#include <variant>

namespace redis {
    enum class RedisError : int16_t {
        success = 0,
        invalid_frame,
        incomplete_frame,
        empty_buffer,
        atoi,
        wrong_arg_types,
    };

    std::ostream &operator<<(std::ostream &o, RedisError err);

    struct RedisErrorCategory final : std::error_category {
        [[nodiscard]] const char *name() const noexcept override { return "RedisError"; }

        [[nodiscard]] std::string message(int c) const override {
            switch (static_cast<RedisError>(c)) {
                case RedisError::success:
                    return "success";
                case RedisError::invalid_frame:
                    return "invalid frame";
                case RedisError::incomplete_frame:
                    return "not enough data to decode a full frame";
                case RedisError::empty_buffer:
                    return "the buffer does not contain any data";
                case RedisError::atoi:
                    return "cannot convert string to integer";
                case RedisError::wrong_arg_types:
                    return "received a wrong frame variant as arg";
            }
            return "redis::RedisError::unknown";
        }
    };

    // Create a global error category instance
    inline const RedisErrorCategory &RedisErrorCategory() noexcept {
        static struct RedisErrorCategory instance;
        return instance;
    }

    // Convert RedisError to std::error_code
    inline std::error_code make_error_code(RedisError e) noexcept {
        return {static_cast<int>(e), RedisErrorCategory()};
    }

    template<typename T>
    struct Result {
        std::variant<T, RedisError> data;

        [[nodiscard]] bool is_error() const noexcept { return std::holds_alternative<RedisError>(data); }

        [[nodiscard]] const T &value() const {
            if (is_error()) {
                throw std::logic_error("cannot get value from an error variant");
            }
            return std::get<T>(data);
        }

        T &value() {
            if (is_error()) {
                throw std::logic_error("cannot get value from an error variant");
            }
            return std::get<T>(data);
        }

        [[nodiscard]] RedisError error() const {
            if (!is_error()) {
                throw std::logic_error("result does not contain an error");
            }
            return std::get<RedisError>(data);
        }

        static Result make_error(const RedisError err) { return Result{err}; }
    };

    constexpr Result<std::string> make_string_error(const RedisError err) noexcept {
        return Result<std::string>::make_error(err);
    }

    constexpr Result<Frame> make_frame_error(const RedisError err) noexcept { return Result<Frame>::make_error(err); }

    constexpr Result<int64_t> make_int64_error(const RedisError err) noexcept {
        return Result<int64_t>::make_error(err);
    }

} // namespace redis

// Specialize the std::is_error_code_enum template for RedisError
namespace std {
    template<>
    struct is_error_code_enum<redis::RedisError> : true_type {};
} // namespace std