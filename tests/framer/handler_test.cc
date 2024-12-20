#include "framer/handler.h"

#include <gtest/gtest.h>
#include <photon/common/alog.h>
#include <photon/common/memory-stream/memory-stream.h>
#include <photon/common/utility.h>
#include <photon/photon.h>

#include "memory_stream/mstream.h"

using namespace redis;
class HandlerTest : public ::testing::Test
{
protected:
    std::unique_ptr<Handler> h;
    std::unique_ptr<MemoryStream> client{nullptr};

    void SetUp() override
    {
        auto dup = MemoryStream::duplex(1024);
        client = std::move(dup.first);
        h = std::make_unique<Handler>(std::move(dup.second), 25);
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
    }
};

bytes string_to_bytes(const std::string& input) { return std::vector(input.begin(), input.end()); }
//
// Read Exact
//
TEST_F(HandlerTest, ReadExact)
{
    // empty buffer handler, nothing in yet
    auto read = h->read_exact(3);
    EXPECT_TRUE(read.is_error());
    EXPECT_EQ(read.error(), RedisError::eof) << "an empty buffer and eof bit set means we are truly EOF";

    auto data = "hello";
    client->send(data, 5);
    auto read1 = h->read_exact(3);
    EXPECT_FALSE(read1.is_error());
    EXPECT_EQ(read1.value(), string_to_bytes("hel")) << "read_exact can read part of a buffer";

    auto read2 = h->read_exact(2);
    EXPECT_EQ(read2.value(), string_to_bytes("lo")) << "read_exact can read part the rest of a buffer";
    ASSERT_TRUE(h->seen_eof()) << "read_exact: the buffer is empty and eof seen in upstream";
}

TEST_F(HandlerTest, ReadExactNotEnoughData)
{
    std::string_view data{"hello1"};
    client->send(data.data(), data.size());
    auto read3 = h->read_exact(8);
    ASSERT_TRUE(h->seen_eof()) << "read_exact: reading more than the buffer should set EOF bit";
    EXPECT_TRUE(read3.is_error());
    EXPECT_EQ(read3.error(), RedisError::not_enough_data) << "read_exact: should error when there is not enough data";
}

//
// Read until
//

TEST_F(HandlerTest, ReadUntilBasic)
{
    auto data = "hello\nha";
    client->send(data, 8);
    auto read = h->read_until('\n');
    EXPECT_EQ(read.value(), string_to_bytes("hello\n")) << "read_until can read part of a buffer";
    ASSERT_TRUE(h->seen_eof()) << "read_until: data read is lower than chunk size, so EOF should be set";
    ASSERT_EQ(h->get_buffer(), string_to_bytes("ha"));
}

TEST_F(HandlerTest, ReadUntilMultipleReads)
{
    auto data = "hello\nworld\nouu";
    client->send(data, 15);
    auto read = h->read_until('\n');
    EXPECT_EQ(read.value(), string_to_bytes("hello\n")) << "read_until can read part of a buffer";
    ASSERT_TRUE(h->seen_eof()) << "read_until: data read is lower than chunk size, so EOF should be set";
    ASSERT_EQ(std::string_view(h->get_buffer().begin(), h->get_buffer().end()), "world\nouu");
    auto read2 = h->read_until('\n');
    EXPECT_EQ(read2.value(), string_to_bytes("world\n"))
            << "read_until subsequent reads are still success even if the network is "
               "closed because the buffer hold valid data";
}

TEST_F(HandlerTest, ReadUntilDelimiterAtBoundary)
{
    auto data = "hello\n";
    client->send(data, 6);
    auto read = h->read_until('\n');
    EXPECT_EQ(read.value(), string_to_bytes("hello\n")) << "read_until can read part of a buffer";
    ASSERT_TRUE(h->seen_eof()) << "read_until: data read is lower than chunk size, so EOF should be set";
    ASSERT_TRUE(h->empty());
}

TEST_F(HandlerTest, ReadUntilMultipleChunkLowerThanData)
{
    std::string data;
    for (int i = 0; i < 100; ++i)
    {
        data += "hello\n";
    }
    client->send(data.data(), 600);
    auto read = h->read_until('\n');
    EXPECT_EQ(read.value(), string_to_bytes("hello\n")) << "read_until can read part of a buffer";
    ASSERT_FALSE(h->seen_eof()) << "read_until: chunk is lower than data so EOF should not be set";
}

TEST_F(HandlerTest, ReadUntilRequestSpanMultipleInternalReads)
{
    // Internally, we will need make multiple read call under the hood.
    std::string data;
    for (int i = 0; i < 11; ++i)
    {
        data += "hello";
    }
    data += "\nhahah";
    client->send(data.data(), data.size());
    auto read = h->read_until('\n');
    EXPECT_EQ(read.value().size(), 56) << "read_until can read part of a buffer";
    EXPECT_EQ(std::vector(data.begin(), data.begin() + 56), read.value());
}

TEST_F(HandlerTest, ReadUntilEmptyBufferNoData)
{
    auto read = h->read_until('\n');
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::eof) << "an empty buffer and EOF set to EOF";
}

TEST_F(HandlerTest, ReadUntilNoDelimiter)
{
    std::string data = "hello";
    client->send(data.data(), data.size());
    auto read = h->read_until('\n');
    ASSERT_TRUE(read.is_error());
    //@TODO change the error to delimiter not found
    ASSERT_EQ(read.error(), RedisError::incomplete_frame) << "the delimiter does not exist in this stream";
}

TEST_F(HandlerTest, DecodeIntAtBoundary)
{
    const std::string data = ":25\r\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read.is_error());
    auto frame = Frame{FrameID::Integer, 25};
    ASSERT_EQ(read.value(), frame);
}

TEST_F(HandlerTest, DecodeIntInTheMiddle)
{
    const std::string data = ":0\r\nheloe";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read.is_error());
    auto frame = Frame{FrameID::Integer, 0};
    ASSERT_EQ(read.value(), frame);
}

TEST_F(HandlerTest, DecodeIntNagative)
{
    const std::string data = ":-25\r\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read.is_error());
    auto frame = Frame{FrameID::Integer, -25};
    ASSERT_EQ(read.value(), frame);
}

TEST_F(HandlerTest, DecodeIntAtoi)
{
    const std::string data = ":-aeQ\r\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::atoi);
}

TEST_F(HandlerTest, DecodeSimpleIncomplete)
{
    const std::string data = ":\r";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::incomplete_frame);
}

TEST_F(HandlerTest, DecodeSimpleInvalid)
{
    const std::string data = ":T\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::invalid_frame);
}

TEST_F(HandlerTest, DecodeSimpleInvalid2)
{
    const std::string data = "+hel\rlo\r\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::invalid_frame);
}

TEST_F(HandlerTest, DecodeSimpleInvalid3)
{
    const std::string data = "+hel\nlo\r\n";
    client->send(data.data(), data.size());
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::invalid_frame);
}

TEST_F(HandlerTest, DecodeSimpleEoF)
{
    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read.is_error());
    ASSERT_EQ(read.error(), RedisError::eof);
}

TEST_F(HandlerTest, DecodeBulk)
{
    const std::string data = "$5\r\nhello\r\n$6\r\nhel\rlo\r\n$6\r\nhel\nlo\r\n$6\r\nhellojj\r";
    client->send(data.data(), data.size());

    auto b_string1 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(b_string1.is_error());
    auto frame = Frame{FrameID::BulkString, string_to_bytes("hello")};
    ASSERT_EQ(b_string1.value(), frame);

    auto b_string2 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(b_string2.is_error());
    auto frame2 = Frame{FrameID::BulkString, string_to_bytes("hel\rlo")};
    ASSERT_EQ(b_string2.value(), frame2);

    auto b_string3 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(b_string3.is_error());
    auto frame3 = Frame{FrameID::BulkString, string_to_bytes("hel\nlo")};
    ASSERT_EQ(b_string3.value(), frame3);

    auto b_string4 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(b_string4.is_error());
    ASSERT_EQ(b_string4.error(), RedisError::invalid_frame);
}

TEST_F(HandlerTest, DecodeBool)
{
    const std::string data = "#t\r\n#f\r\n#u\r\n";
    client->send(data.data(), data.size());

    auto bool1 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(bool1.is_error());
    auto frame = Frame{FrameID::Boolean, true};
    ASSERT_EQ(bool1.value(), frame);

    auto bool2 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(bool2.is_error());
    auto frame2 = Frame{FrameID::Boolean, false};
    ASSERT_EQ(bool2.value(), frame2);

    auto bool3 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(bool3.is_error());
    ASSERT_EQ(bool3.error(), RedisError::invalid_frame);
}

TEST_F(HandlerTest, DecodeSimple)
{
    const std::string data = "+hello\r\n+-25\r\n-hello\r\n";
    client->send(data.data(), data.size());

    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read.is_error());
    auto frame = Frame{FrameID::SimpleString, string_to_bytes("hello")};
    ASSERT_EQ(read.value(), frame);

    auto read2 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read2.is_error());
    auto frame2 = Frame{FrameID::SimpleString, string_to_bytes("-25")};
    ASSERT_EQ(read2.value(), frame2);

    auto read3 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read3.is_error());
    auto frame3 = Frame{FrameID::SimpleError, string_to_bytes("hello")};
    ASSERT_EQ(read3.value(), frame3);
}

TEST_F(HandlerTest, DecodeNull)
{
    const std::string data = "_\r\n_f\r\n$u\r\n";
    client->send(data.data(), data.size());

    auto read = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_FALSE(read.is_error());
    auto frame = Frame{FrameID::Null, std::monostate{}};
    ASSERT_EQ(read.value(), frame);

    auto read2 = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(read2.is_error());
    ASSERT_EQ(read2.error(), RedisError::invalid_frame);
}


TEST_F(HandlerTest, DecodeArray)
{
    const std::string data = "*3\r\n:1\r\n+Two\r\n$5\r\nThree\r\n*2\r\n:1\r\n*1\r\n+Three\r\n*1\r\n$4\r\nPING\r\n";
    client->send(data.data(), data.size());

    const auto result = h->decode(0, MAX_RECURSION_DEPTH);
    auto vect = std::vector{Frame{FrameID::Integer, 1}, Frame{FrameID::SimpleString, string_to_bytes("Two")},
                            Frame{FrameID::BulkString, string_to_bytes("Three")}};
    const auto ans = Frame{FrameID::Array, std::move(vect)};
    EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";

    const auto result2 = h->decode(0, MAX_RECURSION_DEPTH);
    auto inner_vect = std::vector{Frame{FrameID::SimpleString, string_to_bytes("Three")}};
    auto vect2 = std::vector{Frame{FrameID::Integer, 1}, Frame{FrameID::Array, inner_vect}};
    const auto ans2 = Frame{FrameID::Array, std::move(vect2)};
    EXPECT_EQ(result2.value(), ans2) << "can decode a nested array";
}

TEST_F(HandlerTest, DecodeArrayOverflow)
{
    const std::string data = "*2\r\n:1\r\n*1\r\n+Three\r\n*1\r\n$4\r\nPING\r\n";
    client->send(data.data(), data.size());

    const auto result = h->decode(0, 1);
    ASSERT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), RedisError::max_recursion_depth) << "can spot an array overflow";
}

TEST_F(HandlerTest, DecodeArrayIncomplet)
{
    const std::string data = "*3\r\n:1\r\n+Two\r\n$5\r\nThree";
    client->send(data.data(), data.size());

    const auto result = h->decode(0, MAX_RECURSION_DEPTH);
    ASSERT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), RedisError::not_enough_data) << "can spot an incomplete array";
}

TEST_F(HandlerTest, DecodeCommand)
{
    // try decoding a resp command, i.e array of bulk
    const std::string data = "*1\r\n$4\r\nPING\r\n";
    client->send(data.data(), data.size());

    const auto result = h->decode(0, MAX_RECURSION_DEPTH);
    auto vect = std::vector{Frame{FrameID::BulkString, string_to_bytes("PING")}};
    const auto ans = Frame{FrameID::Array, std::move(vect)};
    EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
}

int main(int argc, char** argv)
{
    log_output_level = ALOG_INFO;
    photon::init(photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE);
    DEFER(photon::fini(););
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
