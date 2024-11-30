#include <photon/common/alog.h>
#include <photon/net/socket.h>
#include <photon/thread/std-compat.h>
#include <photon/thread/workerpool.h>


void handle_connection(photon::net::ISocketStream* stream)
{
    int cpu = sched_getcpu();
    LOG_INFO("Handling connection from CPU %d", cpu);
    DEFER(delete stream);
    std::vector<char> buf;  // Predefined buffer size
    buf.reserve(1024);
    while (true)
    {
        ssize_t ret = stream->recv(buf.data(), 1024);
        if (ret == 0)
        {
            LOG_INFO("Connection closed");
            return;
        }
        if (ret < 0)
        {
            LOG_FATAL("failed to bind tcp socket");
            // LOG_ERRNO_RETURN(0,, "failed to read socket");
        }
        // Echo the received message back to the client
        for (size_t i = 0; i < ret; ++i)
        {
            buf[i] = std::toupper(buf[i]);
        }
        stream->send(buf.data(), ret);
        LOG_INFO("Received and sent back ", ret, " bytes. ");
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
    // @TODO: If you want to listen on multiple CPUs ?
    // auto ret = photon_std::work_pool_init(8, photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE);
    // if (ret != 0)
    // {
    //     LOG_FATAL("Work-pool init failed");
    //     abort();
    // }
    DEFER(photon_std::work_pool_fini());

    auto server = photon::net::new_tcp_socket_server();
    if (server == nullptr)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to create tcp server");
    }
    DEFER(delete server);
    photon::net::IPAddr addr{"127.0.0.300"};
    if (server->bind(6379, addr) < 0)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to bind server to port 9527");
    }
    LOG_INFO("Server is listening for port ", server->getsockname());

    if (server->listen() != 0)
    {
        LOG_ERRNO_RETURN(0, -1, "failed to listen");
    }
    LOG_INFO("Server is listening for port ` ...", 9528);

    photon::WorkPool wp(1, photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE, 128 * 1024);

    while (true)
    {
        auto stream = server->accept();
        if (stream == nullptr)
        {
            LOG_ERRNO_RETURN(0, -1, "failed to accept connection");
        }

        // Add connection handling tasks to the work pool
        wp.async_call(new auto([con = std::move(stream)] { handle_connection(con); }));
    }
    return 0;
}
