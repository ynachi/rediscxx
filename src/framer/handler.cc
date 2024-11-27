//
// Created by ynachi on 10/6/24.
//

#include "framer/handler.h"

#include <algorithm>
#include <charconv>
#include <photon/common/alog.h>


namespace redis
{
    constexpr char CR = '\r';
    constexpr char LF = '\n';

    Handler::Handler(std::unique_ptr<photon::net::ISocketStream> stream, const size_t chunk_size) :
        chunk_size_(chunk_size), stream_(std::move(stream))
    {
        buffer_.reserve(chunk_size_ * 2);
    }

    Result<ssize_t> Handler::get_more_data_upstream_()
    {
        char read_buffer[chunk_size_];
        const auto rd = stream_->recv(read_buffer, chunk_size_);
        if (rd < 0)
        {
            LOG_WARN("failed to read from stream, error: {}", rd);
            return make_ssizet_error(RedisError::generic_network_error);
        }
        if (rd == 0 && buffer_.empty()) return make_ssizet_error(RedisError::eof);
        if (rd < chunk_size_)
        {
            // getting less than chunk_size means we got EOF
            eof_reached_ = true;
        }
        this->add_more_data(std::span(read_buffer, rd));
        return {rd};
    }


    Result<std::string> Handler::read_until(const char c)
    {
        if (this->empty() && this->seen_eof())
        {
            return make_string_error(RedisError::eof);
        }

        int64_t cursor{0};
        for (;;)
        {
            if (auto it = std::ranges::find(buffer_.begin() + cursor, buffer_.end(), c); it != buffer_.end())
            {
                std::string line{buffer_.begin(), it + 1};
                buffer_.erase(buffer_.begin(), it + 1);
                return {line};
            }
            // LOG_DEBUG("could not find the delimiter in the internal buffer, calling more from source stream");
            if (eof_reached_)
            {
                const auto err = buffer_.empty() ? RedisError::eof : RedisError::incomplete_frame;
                return make_string_error(err);
            }
            cursor = static_cast<int64_t>(this->buffer_size());
            // read more data from upstream
            auto maybe_error = this->get_more_data_upstream_();
            if (maybe_error.is_error())
            {
                return make_string_error(maybe_error.error());
            }
            LOG_DEBUG("poured {} bytes from source stream", maybe_error.value());
        }
    }

    Result<std::string> Handler::read_exact(const int64_t n)
    {
        assert(n > 0);
        if (this->empty() && this->seen_eof())
        {
            return make_string_error(RedisError::eof);
        }
        while (buffer_.size() < n && !seen_eof())
        {
            if (auto maybe_error = this->get_more_data_upstream_(); maybe_error.is_error())
            {
                return make_string_error(maybe_error.error());
            }
        }
        if (buffer_.size() < n)
        {
            return make_string_error(RedisError::not_enough_data);
        }
        auto ans = std::string(buffer_.begin(), buffer_.begin() + n);
        buffer_.erase(buffer_.begin(), buffer_.begin() + n);
        return {ans};
    }

    Result<std::string> Handler::get_simple_string_()
    {
        const auto maybe_ans = this->read_until(LF);
        if (maybe_ans.is_error())
        {
            return make_string_error(maybe_ans.error());
        }
        auto ans_view = std::string_view(maybe_ans.value());
        if (ans_view.size() < 2)
        {
            LOG_DEBUG("get_simple_string: data is less than 2 bytes");
            return make_string_error(RedisError::incomplete_frame);
        };
        if (ans_view[ans_view.size() - 2] != CR)
        {
            LOG_DEBUG("get_simple_string: found a standalone LF in the frame, this should not be in simple frames");
            return make_string_error(RedisError::invalid_frame);
        }
        if (std::ranges::find(ans_view.begin(), ans_view.end() - 2, CR) != ans_view.end() - 2)
        {
            LOG_DEBUG("get_simple_string: found a standalone CR in the frame, this should not be in simple frames");
            return make_string_error(RedisError::invalid_frame);
        }
        return {std::string(ans_view.data(), ans_view.size() - 2)};
    }

    Result<int64_t> Handler::get_integer_()
    {
        const auto maybe_ans = this->get_simple_string_();
        if (maybe_ans.is_error())
        {
            return make_int64_error(maybe_ans.error());
        }
        int64_t ans;
        auto [_, ec] =
                std::from_chars(maybe_ans.value().data(), maybe_ans.value().data() + maybe_ans.value().size(), ans);
        if (ec == std::errc())
        {
            return Result<int64_t>(ans);
        }
        return make_int64_error(RedisError::atoi);
    }


    Result<std::string> Handler::get_bulk_string_()
    {
        auto size_result = this->get_integer_();
        if (size_result.is_error())
        {
            return make_string_error(size_result.error());
        }
        auto size = size_result.value();
        if (size == -1)
        {
            // if size == -1, the user intent was to specially send empty bulk frame
            return {""};
        }
        if (size == 0)
        {
            // LOG_ERROR("decode: got a bulk frame with size 0");
            return {RedisError::invalid_frame};
        }

        // also read the CRLF, hence, size + 2
        auto interim_read = read_exact(size + 2);
        if (interim_read.is_error())
        {
            return make_string_error(interim_read.error());
        }

        auto ans_view = std::string_view(interim_read.value());


        if (ans_view[ans_view.size() - 2] != CR || ans_view[ans_view.size() - 1] != LF)
        {
            return make_string_error(RedisError::invalid_frame);
        }

        return Result<std::string>(std::string(ans_view.data(), ans_view.size() - 2));
    }

    Result<FrameID> Handler::get_frame_id_()
    {
        if (!this->seen_eof())
        {
            if (auto maybe_err = this->get_more_data_upstream_(); maybe_err.is_error())
            {
                return {maybe_err.error()};
            }
        }
        if (this->empty())
        {
            return {RedisError::eof};
        }
        auto c = this->buffer_[0];
        this->buffer_.erase(this->buffer_.begin());
        return {frame_id_from_u8(c)};
    }

    Result<Frame> Handler::decode(const u_int8_t dept, const u_int8_t max_depth)
    {
        if (dept >= max_depth)
        {
            return {RedisError::max_recursion_depth};
        }
        const auto maybe_id = this->get_frame_id_();
        if (maybe_id.is_error())
        {
            return {maybe_id.error()};
        }
        switch (const auto id = maybe_id.value())
        {
            case FrameID::Integer:
            {
                const auto int_ans = this->get_integer_();
                if (int_ans.is_error())
                {
                    return {int_ans.error()};
                }
                return {Frame{id, int_ans.value()}};
            }
            case FrameID::SimpleString:
            case FrameID::SimpleError:
            case FrameID::BigNumber:
            {
                const auto simple_str_ans = this->get_simple_string_();
                if (simple_str_ans.is_error())
                {
                    LOG_DEBUG("decode: got an error while decoding a simple frame variant: {}", simple_str_ans.error());
                    return {simple_str_ans.error()};
                }
                return {Frame{id, simple_str_ans.value()}};
            }
            case FrameID::Null:
                return get_null_frame_();
            case FrameID::Boolean:
                return get_bool_frame_();
            case FrameID::BulkString:
            case FrameID::BulkError:
            {
                const auto content = this->get_bulk_string_();
                if (content.is_error())
                {
                    return {content.error()};
                }
                return {Frame{id, content.value()}};
            }
            case FrameID::Array:
            {
                return decode_array_(dept, max_depth);
            }
            default:
                return {Frame{FrameID::Undefined, std::monostate{}}};
        }
    }

    Result<Frame> Handler::get_null_frame_()
    {
        const auto ans = this->get_simple_string_();
        if (ans.is_error())
        {
            return {ans.error()};
        }
        if (const std::string & ans_value{ans.value()}; !ans_value.empty())
        {
            LOG_DEBUG("decode: got a non-null frame with data");
            return {RedisError::invalid_frame};
        }
        return {Frame{FrameID::Null, std::monostate{}}};
    }

    Result<Frame> Handler::get_bool_frame_()
    {
        const auto data = this->get_simple_string_();
        if (data.is_error())
        {
            return {data.error()};
        }
        std::string_view ans_view{data.value()};
        if (ans_view == "t")
        {
            return {Frame{FrameID::Boolean, true}};
        }
        if (ans_view == "f")
        {
            return {Frame{FrameID::Boolean, false}};
        }
        LOG_DEBUG("decode: got a bool frame with data other than bool");
        return {RedisError::invalid_frame};
    }

    Result<Frame> Handler::decode_array_(const u_int8_t dept, const u_int8_t max_depth)
    {
        const auto size_result = this->get_integer_();
        if (size_result.is_error())
        {
            return {size_result.error()};
        }
        const auto size = size_result.value();
        std::vector<Frame> frames;
        frames.reserve(size);
        for (int64_t i = 0; i < size; ++i)
        {
            const auto frame = this->decode(dept + 1, max_depth);
            if (frame.is_error())
            {
                return {frame.error()};
            }
            frames.emplace_back(frame.value());
        }
        return {Frame{FrameID::Array, frames}};
    }

}  // namespace redis
