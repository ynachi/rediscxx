//
// Created by ynachi on 10/13/24.
//

#ifndef MSTREAM_H
#define MSTREAM_H

#include <photon/common/utility.h>

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
        explicit MemoryStream(const size_t capacity) : capacity_(capacity), m_ringbuf_(capacity) {}

        // Implementation of ISocketStream methods
        ssize_t read(void* buf, size_t count) override;
        ssize_t write(const void* buf, size_t count) override;
        ssize_t readv(const struct iovec* iov, int iovcnt) override;
        ssize_t writev(const struct iovec* iov, int iovcnt) override;

        ssize_t recv(void* buf, size_t count, int flags) override;
        ssize_t recv(const struct iovec* iov, int iovcnt, int flags) override;

        ssize_t send(const void* buf, size_t count, int flags = 0) override;
        ssize_t send(const struct iovec* iov, int iovcnt, int flags = 0) override;

        ssize_t sendfile(int in_fd, off_t offset, size_t count) override;

        int close() override;
        int setsockopt(int level, int option_name, const void* option_value, socklen_t option_len) override;
        int getsockopt(int level, int option_name, void* option_value, socklen_t* option_len) override;
        Object* get_underlay_object(uint64_t recursion) override;

        // Implementation of ISocketName methods
        int getsockname(photon::net::EndPoint& addr) override;
        int getsockname(char* path, size_t count) override;
        int getpeername(photon::net::EndPoint& addr) override;
        int getpeername(char* path, size_t count) override;

    private:
        size_t capacity_ = 1024;
        RingBuffer m_ringbuf_;
        bool closed_ = false;
    };
}  // namespace redis

#endif  // MSTREAM_H
