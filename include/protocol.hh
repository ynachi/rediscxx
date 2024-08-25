#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <deque>
#include <expected>
#include <seastar/core/temporary_buffer.hh>
#include "frame.hh"

namespace redis {

    enum class FrameDecodeError {
        Invalid,
        Incomplete,
        Empty,
        Atoi,
        WrongArgsType, // calling a function with unexpected arguments
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

        /**
         * @brief _read_simple_string tries to read a simple string from the buffer pool.
         *  In case of an error, it strip out the faulty bytes so that the next read do
         *  not contain them.
         *
         * @return the string without advancing the internal cursor of the buffer. The caller should
         * make sure to consume the data read out if needed.
         */
        std::expected<std::string, FrameDecodeError> _read_simple_string() noexcept;

        std::expected<std::string, FrameDecodeError> _read_bulk_string(size_t n) noexcept;

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
         * @brief get_current_buffer_size returns the size of the actual internal buffer.
         *
         * @return
         */
        size_t get_current_buffer_size() {
            if (_data.empty())
                return 0;
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
         * size. n should be the index of the next char to be processed.
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
         * @brief advance consume away (throw) n bytes from the buffer
         *
         * @param n
         */
        // @TODO, make advance returns the number of bytes effectively advanced, in case n > number of available chars
        void advance(size_t n) noexcept;

        std::expected<int64_t, FrameDecodeError> get_int() noexcept;

        /**
         * @brief get_total_size returns the actual number of bytes in the BufferManager
         *
         * @return
         */
        size_t get_total_size() noexcept;

        /**
         * @brief get_simple_frame_variant decodes a frame which has a simple string as internal data.
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
    };
} // namespace redis

#endif // PROTOCOL_H
