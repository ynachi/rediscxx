//
// Created by ynachi on 10/6/24.
//

#ifndef HANDLER_H
#define HANDLER_H

#include <errors.h>
#include <optional>
#include <photon/net/socket.h>
#include <span>
#include <vector>

#include "frame.h"

namespace redis
{
    /**
     *
     */
    class Handler
    {
    public:
        Handler(const Handler&) = delete;
        Handler& operator=(const Handler&) = delete;
        Handler(Handler&&) = delete;
        Handler& operator=(Handler&&) = delete;

        Handler(std::unique_ptr<photon::net::ISocketStream> stream, size_t chunk_size);

        /**
         * is_eof checks for real eof meaning the buffer is empty and the eof bit was seen on the upstream stream.
         */
        bool is_eof() const noexcept { return buffer_.empty() && eof_reached_; }

        /**
         * read_until read from the handler buffer or/and the upstream stream until char c is reached.
         * This method consume the buffer.
         * @return the bytes read as a string or an error.
         * */
        Result<std::string> read_until(char c);

        /**
         * read_exact attempt to read exact n bytes from the handler buffer or/and the upstream stream.
         * This method consume the buffer.
         * @return the bytes read as a string.
         * */
        Result<std::string> read_exact(int64_t n);

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

        Result<Frame> read_simple_frame();

        void add_more_data(std::span<char> bytes) noexcept
        {
            buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        }

    private:
        std::optional<RedisError> get_more_data_upstream_();
        Result<std::string> read_simple_string_();
        Result<int64_t> read_integer_();
        Result<std::string> read_bulk_string_();

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
