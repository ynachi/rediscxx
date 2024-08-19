#include <iostream>
#include <protocol.hh>
#include <stdexcept>

namespace redis {
    void Decoder::add_upstream_data(seastar::temporary_buffer<char> chunk) noexcept {
        this->_data.emplace_back(std::move(chunk));
    }

    std::expected<std::string, FrameDecodeError> Decoder::get_simple_string() noexcept {
        auto positions = this->_read_until_crlf_simple();
        if (!positions.has_value()) {
            // @TODO: drop faulty data if error is not FrameDecodeError::Incomplete
            return std::unexpected(positions.error());
        }
        std::string result;

        // append full buffers as it
        for (; positions->first > 0; --positions->first) {
            result.append(this->_data.front().get(), this->_data.front().size());
            this->_data.pop_front();
        }

        // now process the partial buffer.
        // -1 means we exclude CRLF
        result.append(this->_data.front().get(), positions->second - 1);

        // remove the processed data or pop the current queue if we've processed all the data of this queue.
        if (positions->second + 1 < this->get_current_buffer_size()) {
            this->_data.front().trim_front(positions->second + 1);
        } else {
            this->reset_positions(0);
        }
        // successful read means reset the cursor to 0 because the data has been consumed
        this->_data.pop_front();
        return result;
    }

    std::expected<std::pair<size_t, size_t>, FrameDecodeError> Decoder::_read_until_crlf_simple() noexcept {
        if (this->_data.empty()) {
            return std::unexpected(FrameDecodeError::Empty);
        }

        size_t buffer_index = 0;
        while (buffer_index < this->get_buffer_number()) {
            auto &current_buf = _data[buffer_index];
            if (current_buf.size() < 2) {
                return std::unexpected(FrameDecodeError::Incomplete);
            }
            for (; this->_cursor_position < current_buf.size(); ++this->_cursor_position) {
                if (current_buf[this->_cursor_position] == '\r') {
                    if (this->_cursor_position == current_buf.size() - 1) {
                        return std::unexpected(FrameDecodeError::Incomplete);
                    }
                    if (current_buf[this->_cursor_position + 1] != '\n') {
                        return std::unexpected(FrameDecodeError::Invalid);
                    }
                    this->_cursor_position += 1; // Move the cursor at the LF of CRLF
                    return std::make_pair(buffer_index, this->_cursor_position);
                }
                if (current_buf[this->_cursor_position] == '\n') {
                    return std::unexpected(FrameDecodeError::Invalid);
                }
            }
            this->_cursor_position = 0; // Reset cursor position for the next buffer
            ++buffer_index;
        }
        return std::unexpected(FrameDecodeError::Incomplete);
    }

    void Decoder::reset_positions(size_t new_cursor_position) {
        if (this->_data.empty() || new_cursor_position >= this->_data[0].size()) {
            throw std::out_of_range("buffer is empty or cursor position out of range");
        }
        this->_cursor_position = new_cursor_position;
    }
} // namespace redis
