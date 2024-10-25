//
// Created by ynachi on 10/6/24.
//

#ifndef HANDLER_H
#define HANDLER_H

#include <expected>
#include <seastar/core/reactor.hh>
#include <seastar/net/api.hh>
#include <seastar/util/log.hh>
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

        Handler(seastar::connected_socket&& stream, const seastar::socket_address& addr,
                seastar::lw_shared_ptr<seastar::logger> logger, size_t chunk_size);

        // append_data appends a temporary buffer to the buffer managed by the handler.
        // The buffer passed to this method would typically come from a stream of network in the case of this
        // application.
        void append_data(seastar::temporary_buffer<char>&& data) noexcept
        {
            buffer_.insert(buffer_.end(), data.begin(), data.end());
        }

        bool eof() const { return input_stream_.eof(); }
        /**
         * write_to_stream writes some data to the underlined stream of the handler.
         * It is similar to calling the send method on the Photonlib socket stream.
         * @param data
         * @param size
         * @return return the same output as send. The number of bytes written if success, a negative number if not.
         */
        ssize_t write_to_stream(const char* data, const size_t size) const { connected_socket_->input() }
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

        size_t chunck_size_ = 1024;
        std::vector<char> buffer_;
        seastar::connected_socket connected_socket_;
        seastar::input_stream<char> input_stream_;
        seastar::output_stream<char> output_stream_;
        seastar::socket_address remote_address_;
        seastar::lw_shared_ptr<seastar::logger> logger_;
        bool eof_reached_ = false;
        size_t read_cursor_ = 0;
    };
}  // namespace redis

#endif  // HANDLER_H
