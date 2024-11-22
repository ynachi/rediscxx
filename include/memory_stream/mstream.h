//
// Created by ynachi on 10/13/24.
//

#ifndef MSTREAM_H
#define MSTREAM_H

#include <photon/common/utility.h>
#include <utility>

#include "photon/common/ring.h"
#include "photon/net/socket.h"
#include "photon/thread/std-compat.h"

namespace redis
{
    /**
     * @class MemoryStream
     * @brief A class representing an in-memory stream that implements the ISocketStream interface.
     *
     * MemoryStream provides methods for reading from and writing to an in-memory buffer using
     * socket-like interfaces from the photon network library. It is purely used for testing.
     *
     * @details
     * The class uses a ring buffer to manage the internal memory for the stream. It supports common
     * socket operations including read, write, readv, writev, recv, send, sendfile, and provides
     * methods to manage socket options and names. All the operations are non-blocking. A MemoryStream is a duplex
     * io by itself so there is no need to create a pair to simulate a bidirectional socket. It is also thread safe
     * as the internal struct, the Photon ring buffer, provide this capability.
     */
    class MemoryStream final : public photon::net::ISocketStream
    {
    public:
        struct SimpleStream : std::enable_shared_from_this<SimpleStream>
        {
            std::vector<char> buffer;
            bool is_closed = false;
            size_t max_buffer_size = 1024;
            photon_std::mutex mu;

            // Delete the move and copy constructors and assignment operators
            SimpleStream(const SimpleStream&) = delete;
            SimpleStream& operator=(const SimpleStream&) = delete;
            SimpleStream(SimpleStream&&) = delete;
            SimpleStream& operator=(SimpleStream&&) = delete;

            explicit SimpleStream(const size_t max_buffer_size) : max_buffer_size(max_buffer_size)
            {
                buffer.reserve(max_buffer_size);
            }

            ssize_t read(void* buf, const size_t count)
            {
                photon_std::lock_guard lock(mu);
                if (is_closed) return 0;
                if (buffer.empty()) return -EAGAIN;
                const size_t real_count = std::min(count, buffer.size());
                std::memcpy(buf, buffer.data(), real_count);
                buffer.erase(buffer.begin(), buffer.begin() + real_count);
                return real_count;
            }

            ssize_t write(const void* buf, size_t count)
            {
                photon_std::lock_guard lock(mu);
                // writing to a closed socket is an error
                if (is_closed) return -1;
                // no enough space, write as much as possible
                const auto real_count = std::min(max_buffer_size - buffer.size(), count);
                // Use reinterpret_cast to cast the void* to a const char*
                const auto char_buf = static_cast<const char*>(buf);
                buffer.insert(buffer.end(), char_buf, char_buf + real_count);
                return real_count;
            }

            std::shared_ptr<SimpleStream> clone() { return shared_from_this(); }
        };

        MemoryStream(std::shared_ptr<SimpleStream> read_end, const std::shared_ptr<SimpleStream>& write_end) :
            read_end_(std::move(read_end)), write_end_(write_end) {};

        // Implementation of ISocketStream methods
        ssize_t read(void* buf, size_t count) override { return this->read_end_->read(buf, count); }

        ssize_t write(const void* buf, size_t count) override { return this->write_end_->write(buf, count); }

        ssize_t readv(const struct iovec* iov, int iovcnt) override
        {
            ssize_t total_bytes_read = 0;
            for (int i = 0; i < iovcnt; ++i)
            {
                ssize_t bytes_read = read(static_cast<char*>(iov[i].iov_base), iov[i].iov_len);
                if (bytes_read <= 0)
                {
                    if (total_bytes_read > 0)
                        return total_bytes_read;
                    else
                        return bytes_read;
                }
                total_bytes_read += bytes_read;
            }
            return total_bytes_read;
        }
        ssize_t writev(const struct iovec* iov, int iovcnt) override
        {
            ssize_t total_bytes_written = 0;
            for (int i = 0; i < iovcnt; ++i)
            {
                ssize_t bytes_written = write(iov[i].iov_base, iov[i].iov_len);
                if (bytes_written < 0)
                {
                    return bytes_written;
                }
                total_bytes_written += bytes_written;
                if (bytes_written < iov[i].iov_len)
                {
                    break;
                }
            }
            return total_bytes_written;
        }

        ssize_t recv(void* buf, size_t count, int flags) override { return read(buf, count); }
        ssize_t recv(const struct iovec* iov, int iovcnt, int flags) override { return readv(iov, iovcnt); }

        ssize_t send(const void* buf, size_t count, int flags = 0) override { return write(buf, count); }
        ssize_t send(const struct iovec* iov, int iovcnt, int flags = 0) override { return writev(iov, iovcnt); }

        ssize_t sendfile(int in_fd, off_t offset, size_t count) override
        {
            // Simplifying assumption: this operation is not supported for in-memory streams.
            return -1;
        }

        int close() override
        {
            photon_std::lock_guard lock(read_end_->mu);
            read_end_->is_closed = true;
            write_end_->is_closed = true;
            return 0;
        }
        int setsockopt(int level, int option_name, const void* option_value, socklen_t option_len) override
        {
            // For simplicity, don't handle socket options
            return 0;
        }
        int getsockopt(int level, int option_name, void* option_value, socklen_t* option_len) override
        {
            // For simplicity, don't handle socket options
            return 0;
        }
        Object* get_underlay_object(uint64_t recursion) override { return nullptr; }

        // Implementation of ISocketName methods
        int getsockname(photon::net::EndPoint& addr) override { return 0; }
        int getsockname(char* path, size_t count) override { return 0; }
        int getpeername(photon::net::EndPoint& addr) override { return 0; }
        int getpeername(char* path, size_t count) override { return 0; }

        static std::pair<std::unique_ptr<MemoryStream>, std::unique_ptr<MemoryStream>> duplex(
                const size_t max_buffer_size)
        {
            auto one = std::make_shared<SimpleStream>(max_buffer_size);
            auto two = std::make_shared<SimpleStream>(max_buffer_size);
            auto stream1 = std::make_unique<MemoryStream>(one->clone(), two->clone());
            auto stream2 = std::make_unique<MemoryStream>(two, one);

            return std::make_pair(std::move(stream1), std::move(stream2));
        }

    private:
        std::shared_ptr<SimpleStream> read_end_;
        std::shared_ptr<SimpleStream> write_end_;
    };
}  // namespace redis

#endif  // MSTREAM_H
