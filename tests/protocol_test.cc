#include <gtest/gtest.h>
#include <photon/common/alog.h>
#include <photon/common/memory-stream/memory-stream.h>
#include <photon/common/utility.h>
#include <photon/photon.h>

#include "framer/handler.h"
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
        h = std::make_unique<Handler>(std::move(dup.second), 1024);
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
    }
};


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
    EXPECT_EQ(read1.value(), "hel") << "read_exact can read part of a buffer";

    auto read2 = h->read_exact(2);
    EXPECT_EQ(read2.value(), "lo") << "read_exact can read part the rest of a buffer";
    ASSERT_TRUE(h->seen_eof()) << "read_exact: the buffer is empty and eof seen in upstream";
}

TEST_F(HandlerTest, ReadExactNotEnoughData)
{
    std::string_view data{"hello1"};
    client->send(data.data(), data.size());
    auto read3 = h->read_exact(8);
    ASSERT_TRUE(h->seen_eof()) << "read_exact: reading more than the buffer should set EOF bit";
    EXPECT_TRUE(read3.is_error());
    EXPECT_EQ(read3.error(), RedisError::not_enough_data) << "read_exact: should error when there is not enough data ";
}

// TEST_F(BufferManagerTest, GetSimpleString)
// {
//     auto& buffer = h.get_buffer();
//     buffer.push_back('A');
//     auto result = h.get_simple_string();
//     EXPECT_EQ(result.error(), FrameDecodeError::Incomplete) << "frame should be terminated by CRLF";
//     EXPECT_EQ(h.get_cursor(), 0) << R"(buffer should not be altered when hitting "incomplete" error type)";
//
//     append_str(buffer, "BC\r");  // os is now "ABC\r"
//     auto result2 = h.get_simple_string();
//     EXPECT_EQ(result2.error(), FrameDecodeError::Incomplete) << "single CR at the end probably means incomplete
//     frame"; EXPECT_EQ(h.get_cursor(), 0) << R"(buffer should not be altered when hitting "incomplete" error type)";
//
//     append_str(buffer, "DEF");  // os is now "ABC\rDEF"
//     auto result3 = h.get_simple_string();
//     EXPECT_EQ(result3.error(), FrameDecodeError::Invalid) << "isolated CR in a simple frame is an error";
//     EXPECT_EQ(h.get_cursor(), 4) << R"(should strip out "ABC\r" and let "DEF")";
//
//     append_str(buffer, "\nDEF");  // os is now "ABC\rDEF\nDEF", cursor is at position 4
//     auto result4 = h.get_simple_string();
//     EXPECT_EQ(result4.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
//     EXPECT_EQ(h.get_cursor(), 8) << R"(should strip out "DEF\n" and let "DEF")";
//
//     append_str(buffer, "\nABC\r\nDEF\r\n55\r\n");  // os is now "ABC\rDEF\nDEF\nABC\r\nDEF\r\n55\r\n"
//     auto result5 = h.get_simple_string();
//     EXPECT_EQ(result5.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
//     EXPECT_EQ(h.get_cursor(), 12) << R"(cursor should be at position 12)";
//
//     h.consume();  // os should be now "ABC\r\nDEF\r\n55\r\n"
//     EXPECT_EQ(h.get_cursor(), 0) << R"("consume" method should set the cursor back to 0)";
//     EXPECT_EQ(buffer.size(), 14) << R"("consume" should have stripped out the first 12 chars)";
//
//     auto result6 = h.get_simple_string();
//     EXPECT_EQ(result6.value(), "ABC") << "can decode a valid frame";
//     EXPECT_EQ(buffer.size(), 14) << R"(should not automatically alter the buffer data)";
//     EXPECT_EQ(h.get_cursor(), 5) << R"("get_simple_string" the cursor should move after the decoded frame if
//     success)";
//
//     auto result7 = h.get_simple_string();
//     EXPECT_EQ(result7.value(), "DEF") << "can decode a valid frame after another in the same stream";
//     EXPECT_EQ(h.get_cursor(), 10) << R"("get_simple_string" can decode another good frame from the same stream)";
//
//     auto result8 = h.get_simple_string();
//     EXPECT_EQ(result8.value(), "55") << "can decode a valid frame at the end of a stream";
//     EXPECT_EQ(h.get_cursor(), 14) << R"("get_simple_string" can decode a frame ending the stream)";
//
//     h.consume();  // os should be now ""
//     EXPECT_EQ(h.get_cursor(), 0) << R"("consume" method should set the cursor back to 0)";
//     EXPECT_EQ(buffer.size(), 0) << R"("consume" should have stripped out everything and should not prduce
//     exception)";
// }
//
// // ****************
// // get_bulk_string
// // **************
// TEST_F(BufferManagerTest, GetBulkString_EmptyData)
// {
//     auto result = h.get_bulk_string(6);
//     EXPECT_EQ(result.error(), FrameDecodeError::Empty);
// }
//
// TEST_F(BufferManagerTest, GetBulkString)
// {
//     auto& buffer = h.get_buffer();
//     buffer.push_back('A');
//     buffer.push_back('\r');
//     buffer.push_back('\n');
//     auto result = h.get_bulk_string(2);
//     EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
//
//     const auto result1 = h.get_bulk_string(1);
//     EXPECT_EQ(result1.value(), "A") << "get_bulk_string: can decode a simple 1 byte frame";
//     EXPECT_EQ(h.get_cursor(), 3) << R"(get_bulk_string: incomplete error variant does not move the cursor)";
//
//     append_str(buffer, "ABC\r");  // buffers is now "A\r\nABC\r"
//     auto result2 = h.get_bulk_string(3);
//     EXPECT_EQ(result2.error(), FrameDecodeError::Incomplete) << "bulk string is not terminated by CRLF";
//     EXPECT_EQ(h.get_cursor(), 3);
//
//     append_str(buffer, "\nDEF\rABC\r\nEF\nXFG\r\n");  // "A\r\nABC\r\nDEF\rABC\r\nEF\nXFG\r\n"
//     const auto result3 = h.get_bulk_string(3);
//     EXPECT_EQ(result3.value(), "ABC") << "get_bulk_string can decode string";
//     EXPECT_EQ(h.get_cursor(), 8);
//
//     auto result4 = h.get_bulk_string(7);
//     EXPECT_EQ(result4.value(), "DEF\rABC") << "get_bulk_string can decode a string with CR in it";
//     EXPECT_EQ(h.get_cursor(), 17);
//
//     auto result5 = h.get_bulk_string(6);
//     EXPECT_EQ(result5.value(), "EF\nXFG") << "get_bulk_string can decode a string with LF in it";
//     EXPECT_EQ(h.get_cursor(), 25);
// }
//
// TEST_F(BufferManagerTest, GetInt)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "25\r\na26\r\n");
//
//     const auto result = h.get_int();
//     EXPECT_EQ(result.value(), 25);
//
//     auto result2 = h.get_int();
//     EXPECT_EQ(result2.error(), FrameDecodeError::Atoi);
// }
//
// TEST_F(BufferManagerTest, GetBool)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "t\r\na26\r\n");
//
//     const auto result = h.get_boolean_frame();
//     const auto ans = Frame{FrameID::Boolean, true};
//     EXPECT_EQ(result.value(), ans);
// }
//
// TEST_F(BufferManagerTest, GetNull)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "t\r\na26\r\n");
//
//     const auto result = h.get_null_frame();
//     const auto ans = Frame{
//             FrameID::Null,
//     };
//     EXPECT_EQ(result.value(), ans);
// }
//
// TEST_F(BufferManagerTest, get_simple_frame_variant)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "ABC\r\na26\r\nt\r\n");
//
//     auto result = h.get_simple_frame_variant(FrameID::SimpleString);
//     auto ans = Frame{FrameID::SimpleString, "ABC"};
//     EXPECT_EQ(result.value(), ans);
//
//     auto result1 = h.get_simple_frame_variant(FrameID::Integer);
//     EXPECT_EQ(result1.error(), FrameDecodeError::WrongArgsType);
//
//     auto result2 = h.get_simple_frame_variant(FrameID::SimpleError);
//     auto ans2 = Frame{FrameID::SimpleError, "a26"};
//     EXPECT_EQ(result2.value(), ans2);
// }
//
// TEST_F(BufferManagerTest, DecodeNonNested)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "+OK\r\n+\r\n-err\n");
//     EXPECT_EQ(buffer.size(), 13);
//
//     auto result = h.decode_frame();
//     auto ans = Frame{FrameID::SimpleString, "OK"};
//     EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
//     EXPECT_EQ(buffer.size(), 8) << "successful decode should consume the buffer";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::SimpleString, ""};
//     EXPECT_EQ(result.value(), ans) << "can decode an empty simple string";
//     EXPECT_EQ(buffer.size(), 5) << "successful decode should consume the buffer";
//
//     result = h.decode_frame();
//     EXPECT_EQ(result.error(), FrameDecodeError::Invalid) << "Simple error frame is malformed, no CRLF";
//     EXPECT_EQ(buffer.size(), 0) << "invalid decode should consume the buffer";
//
//     append_str(buffer, "$5\r\nhello\r\n-err\r\n:66\r\n:-5\r\n:0\r\n#t\r\n#f\r\n#n\r\n!3\r\nerr\r\n");
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::BulkString, "hello"};
//     EXPECT_EQ(result.value(), ans) << "can decode a bulk string";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::SimpleError, "err"};
//     EXPECT_EQ(result.value(), ans) << "can decode a simple error";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::Integer, 66};
//     EXPECT_EQ(result.value(), ans) << "can decode a positive integer";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::Integer, -5};
//     EXPECT_EQ(result.value(), ans) << "can decode a negative integer";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::Integer, 0};
//     EXPECT_EQ(result.value(), ans) << "can decode a 0 as integer";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::Boolean, true};
//     EXPECT_EQ(result.value(), ans) << "can decode a boolean which is true";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::Boolean, false};
//     EXPECT_EQ(result.value(), ans) << "can decode a boolean which is false";
//
//     result = h.decode_frame();
//     EXPECT_EQ(result.error(), FrameDecodeError::Invalid) << "can spot a malformed boolean frame";
//
//     result = h.decode_frame();
//     ans = Frame{FrameID::BulkError, "err"};
//     EXPECT_EQ(result.value(), ans) << "can decode a bulk error";
// }
//
// TEST_F(BufferManagerTest, DecodeSimpleArray)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*3\r\n:1\r\n+Two\r\n$5\r\nThree\r\n");
//
//     const auto result = h.decode_frame();
//     auto vect = std::vector{Frame{FrameID::Integer, 1}, Frame{FrameID::SimpleString, "Two"},
//                             Frame{FrameID::BulkString, "Three"}};
//     const auto ans = Frame{FrameID::Array, std::move(vect)};
//     EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
// }
//
// TEST_F(BufferManagerTest, DecodeNestedArray)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*2\r\n:1\r\n*1\r\n+Three\r\n");
//
//     const auto result = h.decode_frame();
//     auto inner_vect = std::vector{Frame{FrameID::SimpleString, "Three"}};
//     auto vect = std::vector{Frame{FrameID::Integer, 1}, Frame{FrameID::Array, inner_vect}};
//     const auto ans = Frame{FrameID::Array, std::move(vect)};
//     EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
// }
//
// TEST_F(BufferManagerTest, DecodeSimpleArrayOfBulkString)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*1\r\n$4\r\nPING\r\n");
//
//     auto vect = std::vector{Frame{FrameID::BulkString, "PING"}};
//     const auto ans = Frame{FrameID::Array, std::move(vect)};
//
//     auto result = h.decode_frame();
//     EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
// }
//
//
// TEST_F(BufferManagerTest, DecodeSimpleArrayIncomplete)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*3\r\n:1\r\n+Two\r\n$5\r\nThree");
//     EXPECT_EQ(buffer.size(), 23);
//
//     auto result = h.decode_frame();
//     EXPECT_EQ(result.error(), FrameDecodeError::Incomplete) << "can spot incomplete frame array";
//     EXPECT_EQ(buffer.size(), 23) << "incomplete decode should not consume the buffer";
// }
//
//
// TEST_F(BufferManagerTest, DecodeNestedArrayIncomplete)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*2\r\n:1\r\n*1\r\n+Three");
//
//     auto result = h.decode_frame();
//     EXPECT_EQ(result.error(), FrameDecodeError::Incomplete) << "can spot incomplete frame array even when nested";
// }
//
// TEST_F(BufferManagerTest, DecodeSimpleArrayInvalid)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*3\r\n:1\r\n+Two\r\n$5\r\nThreeI\r\n");
//
//     auto result = h.decode_frame();
//     EXPECT_EQ(result.error(), FrameDecodeError::Invalid) << "can spot invalid frame array";
// }
//
// TEST_F(BufferManagerTest, DecodeNestedArrayInvalid)
// {
//     auto& buffer = h.get_buffer();
//     append_str(buffer, "*2\r\n:N\r\n*1\r\n+Three\r\n");
//
//     auto result = h.decode_frame();
//     EXPECT_FALSE(result.has_value());
//     EXPECT_EQ(result.error(), FrameDecodeError::Atoi) << "can spot invalid frame array even when nested";
// }

int main(int argc, char** argv)
{
    log_output_level = ALOG_INFO;
    photon::init(photon::INIT_EVENT_IOURING, photon::INIT_IO_NONE);
    DEFER(photon::fini(););
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
