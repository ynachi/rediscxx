//
// Created by ynachi on 10/13/24.
//

#include "memory_stream/mstream.h"

#include <photon/thread/std-compat.h>

namespace redis
{
    ssize_t MemoryStream::read(void* buf, size_t count)
    {
        if (closed_) return -1;
        if (m_ringbuf_.empty()) return -EAGAIN;  // would block
        size_t real_count = m_ringbuf_.size() > count ? count : m_ringbuf_.size();
        return m_ringbuf_.read(buf, real_count);
    }

    ssize_t MemoryStream::write(const void* buf, size_t count)
    {
        if (closed_) return -1;
        return m_ringbuf_.write(buf, count);
    }

    ssize_t MemoryStream::readv(const struct iovec* iov, int iovcnt)
    {
        if (closed_) return -1;
        if (m_ringbuf_.empty()) return -EAGAIN;  // would block
        return m_ringbuf_.readv(iov, iovcnt);
    }

    ssize_t MemoryStream::writev(const struct iovec* iov, int iovcnt)
    {
        if (closed_) return -1;
        return m_ringbuf_.writev(iov, iovcnt);
    }

    ssize_t MemoryStream::recv(void* buf, size_t count, int flags = 0) { return read(buf, count); }

    ssize_t MemoryStream::recv(const struct iovec* iov, int iovcnt, int flags) { return readv(iov, iovcnt); }


    ssize_t MemoryStream::send(const void* buf, size_t count, int flags) { return write(buf, count); }

    ssize_t MemoryStream::send(const struct iovec* iov, int iovcnt, int flags) { return writev(iov, iovcnt); }

    int MemoryStream::close()
    {
        closed_ = true;
        return 0;
    }

    ssize_t MemoryStream::sendfile(int in_fd, off_t offset, size_t count)
    {
        // Simplifying assumption: this operation is not supported for in-memory streams.
        return -1;
    }

    int MemoryStream::setsockopt(int level, int option_name, const void* option_value, socklen_t option_len)
    {
        // For simplicity, don't handle socket options
        return 0;
    }

    int MemoryStream::getsockopt(int level, int option_name, void* option_value, socklen_t* option_len)
    {
        // For simplicity, don't handle socket options
        return 0;
    }

    Object* MemoryStream::get_underlay_object(uint64_t recursion) { return nullptr; }

    // Implementation of ISocketName methods
    int MemoryStream::getsockname(photon::net::EndPoint& addr) { return 0; }

    int MemoryStream::getsockname(char* path, size_t count) { return 0; }

    int MemoryStream::getpeername(photon::net::EndPoint& addr) { return 0; }

    int MemoryStream::getpeername(char* path, size_t count) { return 0; }

}  // namespace redis
