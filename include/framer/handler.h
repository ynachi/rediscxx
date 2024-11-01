//
// Created by ynachi on 10/6/24.
//

#ifndef HANDLER_H
#define HANDLER_H

#include <expected>
#include <photon/net/socket.h>
#include <vector>

#include "frame.h"

namespace redis
{
    class Handler
    {
    public:
        enum class DecodeError
        {
            Invalid,
            // Incomplete means the caller should ask for more data upstream if possible
            Incomplete,
            Empty,
            Atoi,
            Eof,
            //
            ConnexionReset,
            WrongArgsType,
            UndefinedFrame,
            // non-recoverable network error
            FatalNetworkError,
        };

        Handler(const Handler&) = delete;
        Handler& operator=(const Handler&) = delete;
        Handler(Handler&&) = delete;
        Handler& operator=(Handler&&) = delete;

        Handler(std::unique_ptr<photon::net::ISocketStream> stream, size_t chunk_size);
        /**
         * read_more_from_stream reads some data from the network stream. It is typically called when there is not
         * enough data to decode a full frame.
         * @return an integer representing the number of bytes read or -1 if an error occurred.
         */
        ssize_t read_more_from_stream();
        /**
         * write_to_stream writes some data to the underlined stream of the handler.
         * It is similar to calling the send method on the Photonlib socket stream.
         * @param data
         * @param size
         * @return return the same output as send. The number of bytes written if success, a negative number if not.
         */
        ssize_t write_to_stream(const char* data, const size_t size) const { return stream_->send(data, size); }
        /**
         * data is used to get a non-mutable access to the data managed by the buffer.
         *
         * @return a pointer to the first byte of the underlined buffer.
         */
        [[nodiscard]] const char* data() const { return buffer_.data(); }
        /**
         * data_mut is used to get mutable access to the data managed by the buffer.
         *
         * @return a pointer to the first byte of the underlined buffer.
         */
        [[nodiscard]] char* data_mut() { return buffer_.data(); }
        [[nodiscard]] size_t buffer_size() const { return buffer_.size(); }

        std::expected<Frame, DecodeError> read_simple_frame();

    private:
        std::expected<std::string, DecodeError> read_simple_string_();
        std::expected<int64_t, DecodeError> read_integer_();
        std::expected<std::string, DecodeError> read_bulk_string_();

        // Choose chunk size wisely. Initially, a buffer of 2 * chunk_size will be allocated for reading
        // on the network stream.
        size_t chunk_size_ = 1024;
        std::vector<char> buffer_;
        std::unique_ptr<photon::net::ISocketStream> stream_;
        bool eof_reached_ = false;
        size_t cursor_pos_ = 0;
    };
}  // namespace redis

#endif  // HANDLER_H
