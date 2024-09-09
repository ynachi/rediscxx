#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <boost/asio/ip/address.hpp>
#include <expected>
#include <seastar/core/temporary_buffer.hh>

#include "frame.hh"


namespace redis
{

    enum class FrameDecodeError
    {
        Invalid,
        Incomplete,
        Empty,
        Atoi,
        Eof,
        WrongArgsType,
        UndefinedFrame,
    };

    /**
     * @brief The BufferManager class is responsible for decoding Redis protocol frames.
     *
     * The Protocol Decoder class accumulates data chunks in a deque and processes them
     * to extract complete frames as defined by the Redis protocol.
     * We maintain an internal cursor which denotes where we sit in the next chunk of the process
     */
    class BufferManager
    {
        std::vector<char> buffer_;
        size_t cursor_ = 0;

        // used to decode Array, Map and Set frames. For now, we will only work an array.
        std::expected<Frame, FrameDecodeError> _get_aggregate_frame(FrameID frame_type) noexcept;

        std::expected<Frame, FrameDecodeError> _get_non_aggregate_frame(FrameID frame_id) noexcept;

        // helper method to get a non-aggregate frame

    public:
        BufferManager(const BufferManager &) = delete;
        BufferManager &operator=(const BufferManager &) = delete;

        BufferManager() noexcept = default;
        BufferManager(BufferManager &&other) noexcept = delete;
        BufferManager &operator=(BufferManager &&other) noexcept = delete;

        std::vector<char> &get_buffer() noexcept { return buffer_; };

        [[nodiscard]] size_t get_cursor() const noexcept { return cursor_; };

        // Warning, by setting the cursor at buffer.size(), we simulate vect::end() iterator behavior
        // but the cursor shouldn't be used for indexing the buffer. There are risks of exception using
        // buffer_[cursor_] so don't do that.
        void set_cursor(const size_t n) noexcept { cursor_ = n >= buffer_.size() ? buffer_.size() : n; };

        // consume the data from the beginning up to the cursor position
        void consume()
        {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<int>(cursor_));
            cursor_ = 0;
        }

        // returns the number of chars from the actual beginning of the vector to the actual cursor position
        // very useful in tests
        [[nodiscard]] std::size_t count_left() const noexcept
        {
            return std::distance(buffer_.cbegin(), buffer_.cbegin() + static_cast<int>(cursor_));
        }

        // returns the number of chars from the actual position of the cursor to the end of the buffer
        // very useful in tests
        [[nodiscard]] std::size_t count_right() const noexcept
        {
            return std::distance(buffer_.cbegin() + static_cast<int>(cursor_), buffer_.cend());
        }

        /**
         * @brief Get_simple_string tries to decode a simple string as defined in RESP3.
         * A simple string is a string that doesn't contain any of CR or LF in it.
         * This function
         *
         * @return a string or an error
         */
        std::expected<std::string, FrameDecodeError> get_simple_string() noexcept;

        /**
         * @brief get_bulk_string gets a string from the buffer given it length. The bulk string is also terminated
         * by CRLF so these methods check that and return an error if needed.
         *
         * @param length
         * @return
         */
        std::expected<std::string, FrameDecodeError> get_bulk_string(size_t length) noexcept;

        std::expected<int64_t, FrameDecodeError> get_int() noexcept;

        /**
         * @brief get_simple_frame_variant decodes a frame that has a simple strings as internal data.
         * So for now, it decodes SimpleString, SimpleError, BigNumber.
         *
         * @return
         */
        std::expected<Frame, FrameDecodeError> get_simple_frame_variant(FrameID) noexcept;

        /**
         * @brief get_bulk_frame_variant decodes a frame which has a bulk string as internal data.
         * So for now, it decodes BulkString, BulkError.
         *
         * @return
         */
        std::expected<Frame, FrameDecodeError> get_bulk_frame_variant(FrameID) noexcept;

        std::expected<Frame, FrameDecodeError> get_integer_frame() noexcept;

        std::expected<Frame, FrameDecodeError> get_boolean_frame() noexcept;

        std::expected<Frame, FrameDecodeError> get_null_frame() noexcept;

        std::expected<Frame, FrameDecodeError> get_array_frame() noexcept;

        // get the id of a frame from the buffer. Consume the actual byte.
        // Do not call it on an empty buffer!
        FrameID get_frame_id();

        [[nodiscard]] bool has_data() const noexcept { return cursor_ < buffer_.size(); }

        // decode tries to identify the incoming frame and decode ir
        std::expected<Frame, FrameDecodeError> decode_frame() noexcept;
    };
}  // namespace redis

#endif  // PROTOCOL_H
