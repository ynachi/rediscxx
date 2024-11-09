#include "protocol.hh"
#include <iostream>
#include <set>
#include <string>

namespace redis {
    void BufferManager::add_upstream_data(seastar::temporary_buffer<char> chunk) noexcept {
        this->data_.emplace_back(std::move(chunk));
    }

    BufferManager::BufferManager(seastar::connected_socket &&fd) noexcept :
        fd_(std::move(fd)),
        // @TODO param me
        input_stream_(fd.input(seastar::connected_socket_input_stream_config(1024, 512, 8192))),
        // @TODO param me
        output_stream_(fd.output(8192)) {}


    Result<std::string> BufferManager::read_simple_string_() {
        if (this->data_.empty()) {
            return make_string_error(RedisError::empty_buffer);
        }

        std::string result;
        for (const auto &buf: this->data_) {
            for (int i = 0; i < buf.size(); ++i) {
                switch (const auto c = buf[i]) {
                    case '\r':
                        if (i == buf.size() - 1) {
                            return make_string_error(RedisError::incomplete_frame);
                        }
                        if (buf[i + 1] != '\n') {
                            // trim the faulty data
                            this->advance(result.size() + 1);
                            return make_string_error(RedisError::invalid_frame);
                        }
                        return Result<std::string>{result};
                    case '\n':
                        this->advance(result.size() + 1);
                        return make_string_error(RedisError::invalid_frame);
                    default:
                        result.push_back(c);
                }
            }
        }
        return make_string_error(RedisError::incomplete_frame);
    }

    Result<std::string> BufferManager::read_bulk_string_(const size_t n) {
        if (this->data_.empty()) {
            return make_string_error(RedisError::empty_buffer);
        }

        // Do we have enough data to decode the bulk frame ?
        if (this->get_total_size() < n + 2) {
            return make_string_error(RedisError::incomplete_frame);
        }

        // Let's actually read the data we know is enough
        std::string str_read;
        size_t buffer_index{0};
        for (; buffer_index < this->get_buffer_number(); ++buffer_index) {
            auto &current_buf = data_[buffer_index];
            if (str_read.size() + current_buf.size() <= n + 2) {
                str_read.append(current_buf.get(), current_buf.size());
            } else {
                break;
            }
        }

        // At this point, we know the next buffer has enough data for the rest to decode
        const size_t bytes_num_left = n + 2 - str_read.size();
        str_read.append(this->data_[buffer_index].get(), bytes_num_left);

        // now check if it is ending with CRLF
        if (const auto maybe_crlf = str_read.substr(str_read.length() - 2, 2); maybe_crlf != "\r\n") {
            // throw away the faulty bytes
            this->advance(n + 2);
            return make_string_error(RedisError::invalid_frame);
        }

        str_read.resize(str_read.size() - 2);
        return Result<std::string>{str_read};
    }

    Result<std::string> BufferManager::get_simple_string() {
        auto result = this->read_simple_string_();
        if (result.is_error()) {
            return result;
        }
        this->advance(result.value().size() + 2);
        return result;
    }

    Result<std::string> BufferManager::get_bulk_string(const size_t length) {
        auto result = this->read_bulk_string_(length);
        if (result.is_error()) {
            return result;
        }
        this->advance(length + 2);
        return result;
    }

    size_t BufferManager::get_total_size() const noexcept {
        size_t ans{0};
        for (const auto &buf: this->data_) {
            ans += buf.size();
        }
        return ans;
    }

    void BufferManager::advance(const size_t n) noexcept {
        size_t bytes_to_consume = n;

        while (bytes_to_consume > 0 && !this->data_.empty()) {
            auto &current_buf = this->data_.front();

            if (const size_t buf_size = current_buf.size(); buf_size <= bytes_to_consume) {
                // We need to consume the entire buffer
                bytes_to_consume -= buf_size;
                this->data_.pop_front();
            } else {
                // We need to consume part of this buffer
                current_buf.trim_front(bytes_to_consume);
                bytes_to_consume = 0;
            }
        }
    }

    Result<int64_t> BufferManager::get_int() noexcept {
        auto str_result = this->get_simple_string();
        if (!str_result.is_error()) {
            return make_int64_error(str_result.error());
        }
        try {
            int64_t result = std::stoll(str_result.value());
            return Result<int64_t>{result};
        } catch (const std::exception &_) {
            //@TODO log the exception
            return make_int64_error(RedisError::atoi);
        }
    }

    void BufferManager::trim_front(const size_t n) {
        if (n < this->get_current_buffer_size()) {
            this->data_.front().trim_front(n);
        } else {
            this->pop_front();
        }
    }

    void BufferManager::pop_front() noexcept { this->data_.pop_front(); }

    Result<Frame> BufferManager::get_simple_frame_variant(const FrameID frame_id) {
        if (!std::set{FrameID::SimpleString, FrameID::SimpleError, FrameID::BigNumber}.contains(frame_id)) {
            return make_frame_error(RedisError::wrong_arg_types);
        }
        auto frame_data = this->get_simple_string();
        if (frame_data.is_error()) {
            return make_frame_error(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return Result<Frame>{frame};
    }

    Result<Frame> BufferManager::get_bulk_frame_variant(const FrameID frame_id) {
        if (!std::set{FrameID::BulkString, FrameID::BulkError}.contains(frame_id)) {
            return make_frame_error(RedisError::wrong_arg_types);
        }
        auto size = this->get_int();
        if (size.is_error()) {
            return make_frame_error(size.error());
        }
        auto frame_data = this->get_bulk_string(size.value());
        if (frame_data.is_error()) {
            return make_frame_error(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return Result<Frame>{frame};
    }

    Result<Frame> BufferManager::get_integer_frame() {
        auto frame_data = this->get_int();
        if (frame_data.is_error()) {
            return make_frame_error(frame_data.error());
        }
        Frame frame = Frame::make_frame(FrameID::Integer);
        frame.data = frame_data.value();
        return Result<Frame>{frame};
    }

    Result<Frame> BufferManager::get_boolean_frame() {
        auto frame_data = this->get_simple_string();
        if (frame_data.is_error()) {
            return make_frame_error(frame_data.error());
        }
        if (frame_data.value() != "t" && frame_data.value() != "f") {
            return make_frame_error(RedisError::invalid_frame);
        }
        Frame frame = Frame::make_frame(FrameID::Boolean);
        if (frame_data.value() != "t") {
            frame.data = true;
        } else {
            frame.data = false;
        }
        return Result<Frame>{frame};
    }

    Result<Frame> BufferManager::get_null_frame() {
        if (const auto frame_data = this->get_simple_string(); frame_data.is_error()) {
            return make_frame_error(frame_data.error());
        }
        return Result<Frame>{Frame::make_frame(FrameID::Null)};
    }

    seastar::future<> BufferManager::process_frames() {
        std::cout << "entered process frames"
                  << "\n";
        // the caller and this method needs to make sure the connection
        // object is not out of scope, so use a shared reference from
        // the current connection.
        auto const self = this->get_ptr();
        while (true) {
            if (self->input_stream_.eof()) {
                std::cout << "connection was closed by the user"
                          << "\n";
                break;
            }
            auto tmp_read_buf = co_await self->input_stream_.read();
            if (tmp_read_buf.empty()) {
                std::cout << "connection was closed by the user"
                          << "\n";
                break;
            }
            self->add_upstream_data(std::move(tmp_read_buf));
            auto data = self->get_simple_string();
            if (data.is_error() && data.error() == RedisError::incomplete_frame) {
                continue;
            }
            if (data.is_error()) {
                std::cout << "error decoding frame"
                          << "\n";
                continue;
            }
            const auto &ans = data.value();
            std::cout << data.value() << "\n";
            co_await this->output_stream_.write(data.value());
            co_await this->output_stream_.flush();
        }
        co_await self->output_stream_.close();
        co_return;
    }

} // namespace redis
