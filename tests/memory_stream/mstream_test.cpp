#include "memory_stream/mstream.h"

#include <gtest/gtest.h>

using namespace redis;


TEST(mstream, simple_rw)
{
    auto stream = MemoryStream(1024);
    std::string input = "hello world";
    auto wr = stream.write(input.data(), input.size());
    ASSERT_EQ(input.size(), wr);

    std::vector<char> buffer;
    buffer.resize(1024);
    auto rd = stream.read(buffer.data(), input.size());
    ASSERT_EQ(rd, input.size());
    std::string_view view(buffer.data(), input.size());
    ASSERT_EQ("hello world", view);
}

TEST(mstream, double_rw)
{
    auto stream = MemoryStream(1024);

    std::string input = "hello world";
    auto wr = stream.write(input.data(), input.size());
    ASSERT_EQ(input.size(), wr);

    std::vector<char> buffer;
    buffer.resize(1024);

    auto rd = stream.read(buffer.data(), 5);
    ASSERT_EQ(rd, 5);
    std::string_view view(buffer.data(), 5);
    ASSERT_EQ("hello", view);

    // read again
    rd = stream.read(buffer.data(), 6);
    ASSERT_EQ(rd, 6);
    view = std::string_view{buffer.data(), 6};
    ASSERT_EQ(" world", view);
}

TEST(mstream, empty_r_would_block)
{
    auto stream = MemoryStream(1024);

    std::vector<char> buffer;
    buffer.resize(1024);
    auto rd = stream.read(buffer.data(), 6);
    ASSERT_EQ(rd, -EAGAIN);
}

TEST(mstream, simple_rw_more_than_available)
{
    auto stream = MemoryStream(1024);
    std::string input = "hello world";
    auto wr = stream.write(input.data(), input.size());
    ASSERT_EQ(input.size(), wr);

    std::vector<char> buffer;
    buffer.resize(1024);
    auto rd = stream.recv(buffer.data(), 20, 0);
    ASSERT_EQ(rd, 11);
    std::string_view view(buffer.data(), input.size());
    ASSERT_EQ("hello world", view);
}

TEST(mstream, simple_rw_closed_conn)
{
    auto stream = MemoryStream(1024);
    std::string input = "hello world";
    auto wr = stream.write(input.data(), input.size());
    ASSERT_EQ(input.size(), wr);

    std::vector<char> buffer;
    buffer.resize(1024);
    stream.close();
    auto rd = stream.read(buffer.data(), 20);
    ASSERT_EQ(rd, -1);
}

int main(int argc, char** argv)
{
    photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_DEFAULT);
    DEFER(photon::fini(););
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
