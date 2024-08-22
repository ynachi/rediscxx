#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <deque>
#include <expected>
#include <seastar/core/temporary_buffer.hh>

namespace redis {

    enum class FrameDecodeError {
        Invalid,
        Incomplete,
        Empty,
    };

    /**
     * @brief The BufferManager class is responsible for decoding Redis protocol frames.
     *
     * The BufferManager class accumulates data chunks in a deque and processes them
     * to extract complete frames as defined by the Redis protocol. Each fully processed chunk should be removed
     * from the queue. We maintains an internal cursor which denotes where we sit in the next chunk to process
     * (which is actually queue.front()). In case of a faulty frame, the 'bad' data ois trimmed out.
     */
    class BufferManager {
        std::deque<seastar::temporary_buffer<char>> _data;
        size_t _cursor_position = 0;

        /**
         * @brief _read_until_crlf_simple tries to find the position of CRLF in the buffer list (queue number and
         * cursor position. The caller can use the positions to immediately accumulate a simple string.
         * This method does not alter the internal cursor of the decoder object.
         *
         * @return the coordinates of CRLF (queue index, cursor position) of the next CRLF or an error
         */
        std::expected<std::pair<size_t, size_t>, FrameDecodeError> _read_until_crlf_simple() noexcept;

        std::expected<std::pair<size_t, size_t>, FrameDecodeError> _read_until_crlf_bulk(size_t n) noexcept;

    public:
        // prevent copy because we want to match seastar::temporary_buffer<char> behavior
        // which is not copyable.
        BufferManager(const BufferManager &) = delete;
        BufferManager &operator=(const BufferManager &) = delete;

        BufferManager() noexcept = default;
        BufferManager(BufferManager &&other) noexcept = default;
        BufferManager &operator=(BufferManager &&other) noexcept = default;

        /**
         * @brief Adds a chunk of data to the BufferManager. The decoder is supposed to be filled
         * from an external source (network stream, for instance). We expect this source to
         * produce temp buffers which are then added to the decoder to allow the processing
         * of complete RESP frames.
         *
         * @param chunk A temporary buffer containing a chunk of data to be decoded.
         */
        void add_upstream_data(seastar::temporary_buffer<char> chunk) noexcept;

        /**
         * @brief get_simple_string tries to decode a simple string as defined in RESP3.
         * A simple string is a string which does not contain any of CR or LF in it.
         * This function reset the internal cursor back to its initial state if an error occurs.
         * The internal cursor is set to the next position to read from and the successful data is consumed from the
         * buffer group.
         * This variant checks that there is no single CR or LF before a CRLF and fires an error in such cases.
         *
         * @return a string or an error
         */
        std::expected<std::string, FrameDecodeError> get_simple_string() noexcept;

        /**
         * @brief get_bulk_string gets a string from the buffer given it length. The bulk string is also terminated
         * by CRLF so this methods checks that and returns an error if needed.
         *
         * @param length
         * @return
         */
        std::expected<std::string, FrameDecodeError> get_bulk_string(size_t length) noexcept;

        /**
         * @brief reset_positions sets the cursors at the given positions. This methods checks if the new cursors
         * are in valid boundaries and throw if not.
         *
         * @param new_cursor_position
         */
        void reset_positions(size_t new_cursor_position);

        /**
         * @brief get_current_buffer_size returns the size of the actual internal buffer.
         *
         * @return
         */
        size_t get_current_buffer_size() {
            assert(!_data.empty());
            return _data[0].size();
        };

        /**
         * @brief get_buffer_number returns the number of internal buffers.
         *
         * @return
         */
        size_t get_buffer_number() { return _data.size(); };

        /**
         * @brief advance advance the current buffer by n. It removes the buffer if n height than the actual buffer
         * size. n should be the index of the next char to be processed. For example, for "ABC\r\nOLA", calling
         * trim_front(5) would leave the buffer with "OLA".
         *
         * @param n
         */
        void trim_front(size_t n);

        /**
         * @brief pop_front removes a buffer from the front of the queue. Warning! This method set back the cursor to
         * 0. Because we expect to start reading the next buffer from index 0.
         */
        void pop_front() noexcept;

        /**
         * @brief get_cursor_position returns the current position of the internal cursor.
         *
         * @return
         */
        size_t get_cursor_position() { return _cursor_position; };
    };
} // namespace redis

#endif // PROTOCOL_H
