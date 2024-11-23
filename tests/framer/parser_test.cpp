#include <chrono>
#include <gtest/gtest.h>
#include <photon/net/http/client.h>
#include <photon/thread/std-compat.h>
#include <photon/thread/thread11.h>

#include "framer/handler.h"
#include "memory_stream/mstream.h"
#include "photon/common/alog.h"
#include "photon/io/fd-events.h"
#include "photon/net/curl.h"
#include "photon/net/http/server.h"
#include "photon/net/socket.h"
#include "photon/thread/thread.h"

using namespace redis;

class BufferManagerTest : public ::testing::Test
{
protected:
    Handler* handler_ = nullptr;

    void SetUp() override
    {
        auto [client, server] = MemoryStream::duplex(1024);
        handler_ = new Handler(&server, 1024);
    }

    void TearDown() override {}
};

TEST_F(BufferManagerTest, GetSimpleString_EmptyData)
{
    const std::string data = "Hello World!\r\n";
    auto num = handler_->write_to_stream(data.data(), data.size());
    EXPECT_EQ(0, data.length());
    // auto result = handler_->read_simple_frame();
    // EXPECT_EQ(result.error(), Handler::DecodeError::Incomplete);
    // std::this_thread::sleep_for(std::chrono::milliseconds(5));
    // auto num2 = handler_->write_to_stream("hello", 5);
    // EXPECT_EQ(num2, 5);
    // auto read_num = handler_->read_more_from_stream();
    // EXPECT_EQ(read_num, data.length());
    // EXPECT_EQ(handler_->buffer_size(), data.length());
}


int main(int argc, char** argv)
{
    photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_DEFAULT);
    DEFER(photon::fini(););
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
