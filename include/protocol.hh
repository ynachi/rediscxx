#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <boost/asio/ip/address.hpp>
#include <boost/asio/streambuf.hpp>
#include <deque>
#include <expected>
#include <iostream>
#include <seastar/core/temporary_buffer.hh>

#include "frame.hh"

namespace redis {

    inline std::string process_data(const std::string& input) {
        std::string output = input;
        std::ranges::transform(output, output.begin(), ::toupper);
        return output;
    }

    enum class FrameDecodeError {
        Invalid,
        Incomplete,
        Empty,
        Atoi,
        Eof,
        WrongArgsType,
    };

    /**
     * @brief The BufferManager class is responsible for decoding Redis protocol frames.
     *
     * The Protocol Decoder class accumulates data chunks in a deque and processes them
     * to extract complete frames as defined by the Redis protocol. Each fully processed chunk should be removed
     * from the queue. We maintain an internal cursor which denotes where we sit in the next chunk of the process
     * (which is actually queue.front()). In case of a faulty frame, the 'bad' data ois trimmed out.
     */
    class ProtocolDecoder {
        std::vector<char> buffer_;

        /**
         * @brief _read_simple_string tries to read a simple string from the buffer pool.
         *  In case of an error, it strips out the faulty bytes so that the next read do
         *  n't contain them.
         *
         * @return The string without advancing the internal cursor of the buffer. The caller should
         * make sure to consume the data read out if needed.
         */
        std::expected<std::string, FrameDecodeError> _read_simple_string() noexcept;

        std::expected<std::string, FrameDecodeError> _read_bulk_string(size_t n) noexcept;

    public:
        ProtocolDecoder(const ProtocolDecoder &) = delete;
        ProtocolDecoder &operator=(const ProtocolDecoder &) = delete;

        ProtocolDecoder() noexcept = default;
        ProtocolDecoder(ProtocolDecoder &&other) noexcept = delete;
        ProtocolDecoder &operator=(ProtocolDecoder &&other) noexcept = delete;

        std::vector<char>& get_buffer() noexcept {return buffer_;};

        void consume(const int64_t n)
        {
            assert(n <= buffer_.size());
            buffer_.erase(buffer_.begin(), buffer_.begin() + n);
        }

        /**
         * @brief Get_simple_string tries to decode a simple string as defined in RESP3.
         * A simple string is a string that doesn't contain any of CR or LF in it.
         * This function resets the internal cursor back to its initial state if an error occurs.
         * The internal cursor is set to the next position to read from,
         * and the successful data is consumed from the
         * buffer group.
         * This variant checks that there's no single CR or LF before a CRLF and fires an error in such cases.
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
         * @brief get_simple_frame_variant decodes a frame thathas a simple strings as internal data.
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
