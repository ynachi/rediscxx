#include <boost/asio/buffers_iterator.hpp>
#include <iostream>
#include <protocol.hh>
#include <set>
#include <string>

namespace redis {

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::_read_simple_string() noexcept {
        if (this->buffer_.empty())
        {
            return std::unexpected(FrameDecodeError::Empty);
        }

        const size_t size = this->buffer_.size();

        for (int i = 0; i < size; ++i)
        {
            switch (this->buffer_[i])
            {
                case '\r':
                    if (i + 1 > size - 1)
                    {
                        return std::unexpected(FrameDecodeError::Incomplete);
                    }
                    if (this->buffer_[i+1] != '\n')
                    {
                        // Invalid frame. Throw away up to \r included
                        this->consume(i+1);
                        return std::unexpected(FrameDecodeError::Invalid);
                    }
                    return std::string(this->buffer_.begin(), this->buffer_.begin() + i);
                case '\n':
                    // Invalid: isolated LF, consume up to the LF included
                    this->consume(i+1);
                    return std::unexpected(FrameDecodeError::Invalid);
                default:;
            }

        }

        // Incomplete sequence: end of buffer reached without finding CRLF
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    // std::expected<std::string, FrameDecodeError> ProtocolDecoder::_read_bulk_string(const size_t n) noexcept {
    //     if (this->buffer_.size() == 0) {
    //         return std::unexpected(FrameDecodeError::Empty);
    //     }
    //
    //     // Do we have enough data to decode the bulk frame?
    //     if (this->buffer_.size() < n + 2) {
    //         return std::unexpected(FrameDecodeError::Incomplete);
    //     }
    //
    //     // Let's actually read the data we know is enough
    //     const auto internal_buffers = this->buffer_.data();
    //     const auto start = buffers_begin(internal_buffers);
    //     const auto end = buffers_end(internal_buffers);
    //     std::string str_read;
    //     size_t buffer_index{0};
    //     for (; buffer_index < this->get_buffer_number(); ++buffer_index) {
    //         auto &current_buf = _data[buffer_index];
    //         if (str_read.size() + current_buf.size() <= n + 2) {
    //             str_read.append(current_buf.get(), current_buf.size());
    //         } else {
    //             break;
    //         }
    //     }
    //
    //     // At this point, we know the next buffer has enough data for the rest to decode
    //     size_t bytes_num_left = n + 2 - str_read.size();
    //     str_read.append(this->_data[buffer_index].get(), bytes_num_left);
    //     auto maybe_crlf = str_read.substr(str_read.length() - 2, 2);
    //
    //     // now check if it is ending with CRLF
    //     if (maybe_crlf != "\r\n") {
    //         // throw away the faulty bytes
    //         this->advance(n + 2);
    //         return std::unexpected(FrameDecodeError::Invalid);
    //     }
    //
    //     str_read.resize(str_read.size() - 2);
    //     return str_read;
    // }

    std::expected<std::string, FrameDecodeError> ProtocolDecoder::get_simple_string() noexcept {
        auto result = this->_read_simple_string();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        this->consume(result->size() + 2);
        return result;
    }

    // std::expected<std::string, FrameDecodeError> ProtocolDecoder::get_bulk_string(size_t length) noexcept {
    //     auto result = this->_read_bulk_string(length);
    //     if (!result.has_value()) {
    //         return std::unexpected(result.error());
    //     }
    //     this->advance(length + 2);
    //     return result;
    // }
    //
    // size_t ProtocolDecoder::get_total_size() noexcept {
    //     size_t ans{0};
    //     for (const auto &buf: this->_data) {
    //         ans += buf.size();
    //     }
    //     return ans;
    // }
    //
    // void ProtocolDecoder::advance(size_t n) noexcept {
    //     size_t bytes_to_consume = n;
    //
    //     while (bytes_to_consume > 0 && !this->_data.empty()) {
    //         auto &current_buf = this->_data.front();
    //         size_t buf_size = current_buf.size();
    //
    //         if (buf_size <= bytes_to_consume) {
    //             // We need to consume the entire buffer
    //             bytes_to_consume -= buf_size;
    //             this->_data.pop_front();
    //         } else {
    //             // We need to consume part of this buffer
    //             current_buf.trim_front(bytes_to_consume);
    //             bytes_to_consume = 0;
    //         }
    //     }
    // }

    // std::expected<int64_t, FrameDecodeError> ProtocolDecoder::get_int() noexcept {
    //     auto str_result = this->get_simple_string();
    //     if (!str_result.has_value()) {
    //         return std::unexpected(str_result.error());
    //     }
    //     try {
    //         int64_t result = std::stoll(str_result.value());
    //         return result;
    //     } catch (const std::exception &_) {
    //         //@TODO log the error
    //         return std::unexpected(FrameDecodeError::Atoi);
    //     }
    // }

    // void ProtocolDecoder::trim_front(size_t n) {
    //     if (n < this->get_current_buffer_size()) {
    //         this->_data.front().trim_front(n);
    //     } else {
    //         this->pop_front();
    //     }
    // }
    //
    // void ProtocolDecoder::pop_front() noexcept { this->_data.pop_front(); }
    //
    // std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_simple_frame_variant(FrameID frame_id) noexcept {
    //     if (!std::set<FrameID>{FrameID::SimpleString, FrameID::SimpleError, FrameID::BigNumber}.contains(frame_id)) {
    //         return std::unexpected(FrameDecodeError::WrongArgsType);
    //     }
    //     auto frame_data = this->get_simple_string();
    //     if (!frame_data.has_value()) {
    //         return std::unexpected(frame_data.error());
    //     }
    //     Frame frame = Frame::make_frame(frame_id);
    //     frame.data = std::move(frame_data.value());
    //     return frame;
    // }

    // std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_bulk_frame_variant(FrameID frame_id) noexcept {
    //     if (!std::set<FrameID>{FrameID::BulkString, FrameID::BulkError}.contains(frame_id)) {
    //         return std::unexpected(FrameDecodeError::WrongArgsType);
    //     }
    //     auto size = this->get_int();
    //     if (!size.has_value()) {
    //         return std::unexpected(size.error());
    //     }
    //     auto frame_data = this->get_bulk_string(size.value());
    //     if (!frame_data.has_value()) {
    //         return std::unexpected(frame_data.error());
    //     }
    //     Frame frame = Frame::make_frame(frame_id);
    //     frame.data = std::move(frame_data.value());
    //     return frame;
    // }
    //
    // std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_integer_frame() noexcept {
    //     auto frame_data = this->get_int();
    //     if (!frame_data.has_value()) {
    //         return std::unexpected(frame_data.error());
    //     }
    //     Frame frame = Frame::make_frame(FrameID::Integer);
    //     frame.data = frame_data.value();
    //     return frame;
    // }
    //
    // std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_boolean_frame() noexcept {
    //     auto frame_data = this->get_simple_string();
    //     if (!frame_data.has_value()) {
    //         return std::unexpected(frame_data.error());
    //     }
    //     if (frame_data.value() != "t" && frame_data.value() != "f") {
    //         return std::unexpected(FrameDecodeError::Invalid);
    //     }
    //     Frame frame = Frame::make_frame(FrameID::Boolean);
    //     if (frame_data.value() != "t") {
    //         frame.data = true;
    //     } else {
    //         frame.data = false;
    //     }
    //     return frame;
    // }
    //
    // std::expected<Frame, FrameDecodeError> ProtocolDecoder::get_null_frame() noexcept {
    //     auto frame_data = this->get_simple_string();
    //     if (!frame_data.has_value()) {
    //         return std::unexpected(frame_data.error());
    //     }
    //     return Frame::make_frame(FrameID::Null);
    // }

} // namespace redis
