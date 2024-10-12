#include <gtest/gtest.h>
#include <photon/net/http/client.h>

#include "framer/handler.h"
#include "photon/common/alog.h"
#include "photon/io/fd-events.h"
#include "photon/net/curl.h"
#include "photon/net/http/server.h"
#include "photon/net/socket.h"
#include "photon/thread/thread.h"

using namespace redis;

photon::net::ISocketServer* new_server(const std::string& ip, const uint16_t port)
{
    auto server = photon::net::new_tcp_socket_server();
    server->timeout(1000UL * 1000);
    // server->setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);
    server->bind(port, photon::net::IPAddr(ip.c_str()));
    server->listen();
    server->set_handler(nullptr);
    server->start_loop(false);
    return server;
}

class BufferManagerTest : public ::testing::Test
{
protected:
    Handler* handler_ = nullptr;
    photon::net::ISocketServer* server_ = nullptr;

    void SetUp() override
    {
        // Prepare a dummy server for tests
        server_ = new_server("127.0.0.1", 8057);

        // Prepare a dummy client/stream for test
        auto client = photon::net::new_tcp_socket_client();
        photon::net::EndPoint ep("127.0.0.1:8057");

        auto stream = client->connect(ep);
        std::unique_ptr<photon::net::ISocketStream> stream_ptr(stream);

        // Create a new handler instance for each test
        handler_ = new Handler(std::move(stream_ptr), 1024);
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
        delete server_;
        delete handler_;
    }
};

TEST_F(BufferManagerTest, GetSimpleString_EmptyData)
{
    const std::string data = "Hello World!\r\n";
    auto num = handler_->write_to_stream(data.data(), data.size());
    EXPECT_EQ(num, data.length());
    auto result = handler_->read_simple_frame();
    EXPECT_EQ(result.error(), Handler::DecodeError::Incomplete);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto num2 = handler_->write_to_stream("hello", 5);
    EXPECT_EQ(num2, 5);
    auto read_num = handler_->read_more_from_stream();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_EQ(read_num, data.length());
}


int main(int argc, char** argv)
{
    photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_DEFAULT);
    DEFER(photon::fini(););
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
