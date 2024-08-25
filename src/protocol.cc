#include <iostream>
#include <protocol.hh>
#include <set>
#include <string>

namespace redis {
    void BufferManager::add_upstream_data(seastar::temporary_buffer<char> chunk) noexcept {
        this->_data.emplace_back(std::move(chunk));
    }


    std::expected<std::string, FrameDecodeError> BufferManager::_read_simple_string() noexcept {
        if (this->_data.empty()) {
            return std::unexpected(FrameDecodeError::Empty);
        }

        std::string result;
        for (const auto &buf: this->_data) {
            for (int i = 0; i < buf.size(); ++i) {
                auto c = buf[i];
                switch (c) {
                    case '\r':
                        if (i == buf.size() - 1) {
                            return std::unexpected(FrameDecodeError::Incomplete);
                        }
                        if (buf[i + 1] != '\n') {
                            // trim the faulty data
                            this->advance(result.size() + 1);
                            return std::unexpected(FrameDecodeError::Invalid);
                        }
                        return result;
                    case '\n':
                        this->advance(result.size() + 1);
                        return std::unexpected(FrameDecodeError::Invalid);
                    default:
                        result.push_back(c);
                }
            }
        }
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    std::expected<std::string, FrameDecodeError> BufferManager::_read_bulk_string(size_t n) noexcept {
        if (this->_data.empty()) {
            return std::unexpected(FrameDecodeError::Empty);
        }

        // Do we have enough data to decode the bulk frame ?
        if (this->get_total_size() < n + 2) {
            return std::unexpected(FrameDecodeError::Incomplete);
        }

        // Let's actually read the data we know is enough
        std::string str_read;
        size_t buffer_index{0};
        for (; buffer_index < this->get_buffer_number(); ++buffer_index) {
            auto &current_buf = _data[buffer_index];
            if (str_read.size() + current_buf.size() <= n + 2) {
                str_read.append(current_buf.get(), current_buf.size());
            } else {
                break;
            }
        }

        // At this point, we know the next buffer has enough data for the rest to decode
        size_t bytes_num_left = n + 2 - str_read.size();
        str_read.append(this->_data[buffer_index].get(), bytes_num_left);
        auto maybe_crlf = str_read.substr(str_read.length() - 2, 2);

        // now check if it is ending with CRLF
        if (maybe_crlf != "\r\n") {
            // throw away the faulty bytes
            this->advance(n + 2);
            return std::unexpected(FrameDecodeError::Invalid);
        }

        str_read.resize(str_read.size() - 2);
        return str_read;
    }

    std::expected<std::string, FrameDecodeError> BufferManager::get_simple_string() noexcept {
        auto result = this->_read_simple_string();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        this->advance(result->size() + 2);
        return result;
    }

    std::expected<std::string, FrameDecodeError> BufferManager::get_bulk_string(size_t length) noexcept {
        auto result = this->_read_bulk_string(length);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        this->advance(length + 2);
        return result;
    }

    size_t BufferManager::get_total_size() noexcept {
        size_t ans{0};
        for (const auto &buf: this->_data) {
            ans += buf.size();
        }
        return ans;
    }

    void BufferManager::advance(size_t n) noexcept {
        size_t bytes_to_consume = n;

        while (bytes_to_consume > 0 && !this->_data.empty()) {
            auto &current_buf = this->_data.front();
            size_t buf_size = current_buf.size();

            if (buf_size <= bytes_to_consume) {
                // We need to consume the entire buffer
                bytes_to_consume -= buf_size;
                this->_data.pop_front();
            } else {
                // We need to consume part of this buffer
                current_buf.trim_front(bytes_to_consume);
                bytes_to_consume = 0;
            }
        }
    }

    std::expected<int64_t, FrameDecodeError> BufferManager::get_int() noexcept {
        auto str_result = this->get_simple_string();
        if (!str_result.has_value()) {
            return std::unexpected(str_result.error());
        }
        try {
            int64_t result = std::stoll(str_result.value());
            return result;
        } catch (const std::exception &_) {
            //@TODO log the error
            return std::unexpected(FrameDecodeError::Atoi);
        }
    }

    void BufferManager::trim_front(size_t n) {
        if (n < this->get_current_buffer_size()) {
            this->_data.front().trim_front(n);
        } else {
            this->pop_front();
        }
    }

    void BufferManager::pop_front() noexcept { this->_data.pop_front(); }

    std::expected<Frame, FrameDecodeError> BufferManager::get_simple_frame_variant(FrameID frame_id) noexcept {
        if (!std::set<FrameID>{FrameID::SimpleString, FrameID::SimpleError, FrameID::BigNumber}.contains(frame_id)) {
            return std::unexpected(FrameDecodeError::WrongArgsType);
        }
        auto frame_data = this->get_simple_string();
        if (!frame_data.has_value()) {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return frame;
    }

    std::expected<Frame, FrameDecodeError> BufferManager::get_bulk_frame_variant(FrameID frame_id) noexcept {
        if (!std::set<FrameID>{FrameID::BulkString, FrameID::BulkError}.contains(frame_id)) {
            return std::unexpected(FrameDecodeError::WrongArgsType);
        }
        auto size = this->get_int();
        if (!size.has_value()) {
            return std::unexpected(size.error());
        }
        auto frame_data = this->get_bulk_string(size.value());
        if (!frame_data.has_value()) {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(frame_id);
        frame.data = std::move(frame_data.value());
        return frame;
    }

    std::expected<Frame, FrameDecodeError> BufferManager::get_integer_frame() noexcept {
        auto frame_data = this->get_int();
        if (!frame_data.has_value()) {
            return std::unexpected(frame_data.error());
        }
        Frame frame = Frame::make_frame(FrameID::Integer);
        frame.data = frame_data.value();
        return frame;
    }

    std::expected<Frame, FrameDecodeError> BufferManager::get_boolean_frame() noexcept {
        auto frame_data = this->get_simple_string();
        if (!frame_data.has_value()) {
            return std::unexpected(frame_data.error());
        }
        if (frame_data.value() != "t" && frame_data.value() != "f") {
            return std::unexpected(FrameDecodeError::Invalid);
        }
        Frame frame = Frame::make_frame(FrameID::Boolean);
        if (frame_data.value() != "t") {
            frame.data = true;
        } else {
            frame.data = false;
        }
        return frame;
    }

    std::expected<Frame, FrameDecodeError> BufferManager::get_null_frame() noexcept {
        auto frame_data = this->get_simple_string();
        if (!frame_data.has_value()) {
            return std::unexpected(frame_data.error());
        }
        return Frame::make_frame(FrameID::Null);
    }

} // namespace redis
