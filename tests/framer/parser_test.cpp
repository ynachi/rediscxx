#include <chrono>
#include <gtest/gtest.h>
#include <photon/net/http/client.h>
#include <photon/thread/std-compat.h>
#include <photon/thread/thread11.h>

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
    photon::net::ISocketStream* client_stream = nullptr;
    photon::net::ISocketStream* server_stream = nullptr;
    photon_std::mutex mu;
    photon_std::condition_variable cv;
    bool connection_established = false;

    void SetUp() override
    {
        // Prepare a dummy server for tests
        const auto server_addr = photon::net::EndPoint(photon::net::IPAddr("127.0.0.1"), 8080);

        // Create server socket
        server_ = photon::net::new_tcp_socket_server();
        server_->bind(server_addr);
        server_->listen();

        photon_std::thread(
                [&]
                {
                    LOG_INFO("************ Test server started *************************");
                    // LOG_INFO("*********** Test server accepted %v *********************", test_steam->getpeername());
                    {
                        photon_std::lock_guard lock(mu);
                        server_stream = server_->accept();  // Blocking call, but in a separate thread
                        connection_established = true;
                    }
                    LOG_INFO("************ Server got a connection *************************");
                    cv.notify_one();
                })
                .detach();
        // Wait for connection to be established
        LOG_INFO("************ Waiting for client *************************");
        {
            photon_std::unique_lock lock(mu);
            cv.wait(lock, [this] { return connection_established; });
        }
        // auto timeout = std::chrono::duration<std::chrono::seconds>(10);
        // while (!connection_established)
        // {
        //     cv.wait(lock, timeout);
        // }

        // Create client socket and connect to the server
        auto client_socket = photon::net::new_tcp_socket_client();
        client_stream = client_socket->connect(server_addr);  // Blocking call
        if (server_stream == nullptr)
        {
            FAIL() << "Server connection failed";
        }

        std::unique_ptr<photon::net::ISocketStream> stream_ptr(server_stream);

        // Create a new handler instance for each test
        handler_ = new Handler(std::move(stream_ptr), 1024);
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
        if (client_stream) client_stream->close();
        if (server_stream) server_stream->close();
        if (server_) server_->terminate();
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
