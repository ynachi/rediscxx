#include <iostream>
#include <protocol.hh>
#include <stdexcept>

namespace redis {
    void BufferManager::add_upstream_data(seastar::temporary_buffer<char> chunk) noexcept {
        this->_data.emplace_back(std::move(chunk));
    }

    std::expected<std::string, FrameDecodeError> BufferManager::get_simple_string() noexcept {
        auto positions = this->_read_until_crlf_simple();
        if (!positions.has_value()) {
            return std::unexpected(positions.error());
        }
        std::string result;

        // append full buffers as it
        for (; positions->first > 0; --positions->first) {
            result.append(this->_data.front().get(), this->_data.front().size());
            this->pop_front();
        }

        // now process the partial buffer.
        // -1 means we exclude CRLF
        result.append(this->_data.front().get(), positions->second - 1);

        // remove the processed data
        this->trim_front(positions->second + 1);

        return result;
    }

    std::expected<std::pair<size_t, size_t>, FrameDecodeError> BufferManager::_read_until_crlf_simple() noexcept {
        if (this->_data.empty()) {
            return std::unexpected(FrameDecodeError::Empty);
        }

        size_t buffer_index{0};
        size_t in_buffer_cursor_position{0};
        while (buffer_index < this->get_buffer_number()) {
            auto &current_buf = _data[buffer_index];
            if (current_buf.size() < 2) {
                return std::unexpected(FrameDecodeError::Incomplete);
            }
            for (; in_buffer_cursor_position < current_buf.size(); ++in_buffer_cursor_position) {
                if (current_buf[in_buffer_cursor_position] == '\r') {
                    if (in_buffer_cursor_position == current_buf.size() - 1) {
                        return std::unexpected(FrameDecodeError::Incomplete);
                    }
                    if (current_buf[in_buffer_cursor_position + 1] != '\n') {
                        // trim the faulty data
                        this->trim_front(in_buffer_cursor_position + 1);
                        return std::unexpected(FrameDecodeError::Invalid);
                    }
                    in_buffer_cursor_position += 1; // Move the cursor at the LF of CRLF
                    return std::make_pair(buffer_index, in_buffer_cursor_position);
                }
                if (current_buf[in_buffer_cursor_position] == '\n') {
                    // trim the faulty data
                    this->trim_front(in_buffer_cursor_position + 1);
                    return std::unexpected(FrameDecodeError::Invalid);
                }
            }
            in_buffer_cursor_position = 0; // Reset cursor position for the next buffer
            ++buffer_index;
        }
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    //    std::expected<std::pair<size_t, size_t>, FrameDecodeError>
    //    BufferManager::_read_until_crlf_bulk(size_t size) noexcept {
    //        if (this->_data.empty()) {
    //            return std::unexpected(FrameDecodeError::Empty);
    //        }
    //
    //        size_t buffer_index{0};
    //        size_t size_so_far{0};
    //        while (_data[buffer_index].size() + size_so_far < size) {
    //            size_so_far += _data[buffer_index].size();
    //            ++buffer_index;
    //        }
    //
    //        auto buffer_index1 = buffer_index;
    //        while (buffer_index1 < this->get_buffer_number()) {
    //            auto &current_buf = _data[buffer_index1];
    //        }
    //
    //        while (buffer_index < this->get_buffer_number()) {
    //            auto &current_buf = _data[buffer_index];
    //            if (current_buf.size() < 2) {
    //                return std::unexpected(FrameDecodeError::Incomplete);
    //            }
    //            for (; this->_cursor_position < current_buf.size(); ++this->_cursor_position) {
    //                if (current_buf[this->_cursor_position] == '\r') {
    //                    if (this->_cursor_position == current_buf.size() - 1) {
    //                        return std::unexpected(FrameDecodeError::Incomplete);
    //                    }
    //                    if (current_buf[this->_cursor_position + 1] != '\n') {
    //                        // trim the faulty data
    //                        this->trim_front(this->_cursor_position + 1);
    //                        return std::unexpected(FrameDecodeError::Invalid);
    //                    }
    //                    this->_cursor_position += 1; // Move the cursor at the LF of CRLF
    //                    return std::make_pair(buffer_index, this->_cursor_position);
    //                }
    //                if (current_buf[this->_cursor_position] == '\n') {
    //                    // trim the faulty data
    //                    this->trim_front(this->_cursor_position + 1);
    //                    return std::unexpected(FrameDecodeError::Invalid);
    //                }
    //            }
    //            this->_cursor_position = 0; // Reset cursor position for the next buffer
    //            ++buffer_index;
    //        }
    //        return std::unexpected(FrameDecodeError::Incomplete);
    //    }

    void BufferManager::trim_front(size_t n) {
        if (n < this->get_current_buffer_size()) {
            this->_data.front().trim_front(n);
        } else {
            this->pop_front();
        }
    }

    void BufferManager::pop_front() noexcept { this->_data.pop_front(); }

    std::expected<std::string, FrameDecodeError> BufferManager::get_bulk_string(size_t length) noexcept {
        // we need to check that we have enough data upfront because we should consume the data
        // only upon success
    }

} // namespace redis
