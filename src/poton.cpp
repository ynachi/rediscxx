#include <iostream>
#include <photon/common/alog.h>
#include <photon/common/string_view.h>
#include <photon/net/socket.h>
#include <photon/thread/std-compat.h>
#include <photon/thread/workerpool.h>


void handle_connection(photon::net::ISocketStream* stream)
{
    std::vector<char> buf(1024);  // Predefined buffer size
    while (true)
    {
        ssize_t ret = stream->recv(buf.data(), buf.size());
        if (ret <= 0)
        {
            LOG_ERRNO_RETURN(0, , "failed to read socket");
        }
        // Echo the received message back to the client
        stream->send(buf.data(), ret);
        LOG_INFO("Received and sent back ", ret, " bytes.");
        photon::thread_yield();
    }
}

int main()
{
    if (const int ret = photon::init(photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE); ret != 0)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to create initialize io event loop");
    }
    DEFER(photon::fini());

    auto server = photon::net::new_tcp_socket_server();
    if (server == nullptr)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to create tcp server");
    }
    DEFER(delete server);

    if (server->bind_v4localhost(9527) != 0)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to bind server to port 9527");
    }

    if (server->listen() != 0)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to listen");
    }
    LOG_INFO("Server is listening for port ` ...", 9528);

    photon::WorkPool wp(std::thread::hardware_concurrency(), photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE,
                        128 * 1024);

    while (true)
    {
        auto stream = server->accept();
        if (stream == nullptr)
        {
            LOG_ERRNO_RETURN(0, -1, "failed to accept connection");
        }

        // Add connection handling tasks to the work pool
        wp.async_call(new auto([&] { handle_connection(stream); }));
    }
    return 0;
}
