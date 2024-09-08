#include "protocol.hh"

#include <gtest/gtest.h>

using namespace redis;
class BufferManagerTest : public ::testing::Test
{
protected:
    BufferManager decoder;

    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right before the destructor).
    }
};

void append_str(std::vector<char>& vec, const char* str) { vec.insert(vec.end(), str, str + std::strlen(str)); }

TEST_F(BufferManagerTest, GetSimpleString_EmptyData)
{
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetSimpleString)
{
    auto& buffer = decoder.get_buffer();
    buffer.push_back('A');
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete) << "frame should be terminated by CRLF";
    EXPECT_EQ(decoder.get_cursor(), 0) << R"(buffer should not be altered when hitting "incomplete" error type)";

    append_str(buffer, "BC\r");  // os is now "ABC\r"
    auto result2 = decoder.get_simple_string();
    EXPECT_EQ(result2.error(), FrameDecodeError::Incomplete) << "single CR at the end probably means incomplete frame";
    EXPECT_EQ(decoder.get_cursor(), 0) << R"(buffer should not be altered when hitting "incomplete" error type)";

    append_str(buffer, "DEF");  // os is now "ABC\rDEF"
    auto result3 = decoder.get_simple_string();
    EXPECT_EQ(result3.error(), FrameDecodeError::Invalid) << "isolated CR in a simple frame is an error";
    EXPECT_EQ(decoder.get_cursor(), 4) << R"(should strip out "ABC\r" and let "DEF")";

    append_str(buffer, "\nDEF");  // os is now "ABC\rDEF\nDEF", cursor is at position 4
    auto result4 = decoder.get_simple_string();
    EXPECT_EQ(result4.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
    EXPECT_EQ(decoder.get_cursor(), 8) << R"(should strip out "DEF\n" and let "DEF")";

    append_str(buffer, "\nABC\r\nDEF\r\n55\r\n");  // os is now "ABC\rDEF\nDEF\nABC\r\nDEF\r\n55\r\n"
    auto result5 = decoder.get_simple_string();
    EXPECT_EQ(result5.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
    EXPECT_EQ(decoder.get_cursor(), 12) << R"(cursor should be at position 12)";

    decoder.consume();  // os should be now "ABC\r\nDEF\r\n55\r\n"
    EXPECT_EQ(decoder.get_cursor(), 0) << R"("consume" method should set the cursor back to 0)";
    EXPECT_EQ(buffer.size(), 14) << R"("consume" should have stripped out the first 12 chars)";

    auto result6 = decoder.get_simple_string();
    EXPECT_EQ(result6.value(), "ABC") << "can decode a valid frame";
    EXPECT_EQ(buffer.size(), 14) << R"(should not automatically alter the buffer data)";
    EXPECT_EQ(decoder.get_cursor(), 5)
            << R"("get_simple_string" the cursor should move after the decoded frame if success)";

    auto result7 = decoder.get_simple_string();
    EXPECT_EQ(result7.value(), "DEF") << "can decode a valid frame after another in the same stream";
    EXPECT_EQ(decoder.get_cursor(), 10) << R"("get_simple_string" can decode another good frame from the same stream)";

    auto result8 = decoder.get_simple_string();
    EXPECT_EQ(result8.value(), "55") << "can decode a valid frame at the end of a stream";
    EXPECT_EQ(decoder.get_cursor(), 14) << R"("get_simple_string" can decode a frame ending the stream)";

    decoder.consume();  // os should be now ""
    EXPECT_EQ(decoder.get_cursor(), 0) << R"("consume" method should set the cursor back to 0)";
    EXPECT_EQ(buffer.size(), 0) << R"("consume" should have stripped out everything and should not prduce exception)";
}

// ****************
// get_bulk_string
// **************
TEST_F(BufferManagerTest, GetBulkString_EmptyData)
{
    auto result = decoder.get_bulk_string(6);
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetBulkString)
{
    auto& buffer = decoder.get_buffer();
    buffer.push_back('A');
    buffer.push_back('\r');
    buffer.push_back('\n');
    auto result = decoder.get_bulk_string(2);
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);

    const auto result1 = decoder.get_bulk_string(1);
    EXPECT_EQ(result1.value(), "A") << "get_bulk_string: can decode a simple 1 byte frame";
    EXPECT_EQ(decoder.get_cursor(), 3) << R"(get_bulk_string: incomplete error variant does not move the cursor)";

    append_str(buffer, "ABC\r");  // buffers is now "A\r\nABC\r"
    auto result2 = decoder.get_bulk_string(3);
    EXPECT_EQ(result2.error(), FrameDecodeError::Incomplete) << "bulk string is not terminated by CRLF";
    EXPECT_EQ(decoder.get_cursor(), 3);

    append_str(buffer, "\nDEF\rABC\r\nEF\nXFG\r\n");  // "A\r\nABC\r\nDEF\rABC\r\nEF\nXFG\r\n"
    const auto result3 = decoder.get_bulk_string(3);
    EXPECT_EQ(result3.value(), "ABC") << "get_bulk_string can decode string";
    EXPECT_EQ(decoder.get_cursor(), 8);

    auto result4 = decoder.get_bulk_string(7);
    EXPECT_EQ(result4.value(), "DEF\rABC") << "get_bulk_string can decode a string with CR in it";
    EXPECT_EQ(decoder.get_cursor(), 17);

    auto result5 = decoder.get_bulk_string(6);
    EXPECT_EQ(result5.value(), "EF\nXFG") << "get_bulk_string can decode a string with LF in it";
    EXPECT_EQ(decoder.get_cursor(), 25);
}

TEST_F(BufferManagerTest, GetInt)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "25\r\na26\r\n");

    const auto result = decoder.get_int();
    EXPECT_EQ(result.value(), 25);

    auto result2 = decoder.get_int();
    EXPECT_EQ(result2.error(), FrameDecodeError::Atoi);
}

TEST_F(BufferManagerTest, GetBool)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "t\r\na26\r\n");

    const auto result = decoder.get_boolean_frame();
    auto ans = Frame{FrameID::Boolean, true};
    EXPECT_EQ(result.value(), ans);
}

TEST_F(BufferManagerTest, GetNull)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "t\r\na26\r\n");

    const auto result = decoder.get_null_frame();
    auto ans = Frame{
            FrameID::Null,
    };
    EXPECT_EQ(result.value(), ans);
}

TEST_F(BufferManagerTest, get_simple_frame_variant)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "ABC\r\na26\r\nt\r\n");

    auto result = decoder.get_simple_frame_variant(FrameID::SimpleString);
    auto ans = Frame{FrameID::SimpleString, "ABC"};
    EXPECT_EQ(result.value(), ans);

    auto result1 = decoder.get_simple_frame_variant(FrameID::Integer);
    EXPECT_EQ(result1.error(), FrameDecodeError::WrongArgsType);

    auto result2 = decoder.get_simple_frame_variant(FrameID::SimpleError);
    auto ans2 = Frame{FrameID::SimpleError, "a26"};
    EXPECT_EQ(result2.value(), ans2);
}

TEST_F(BufferManagerTest, DecodeNonNested)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "+OK\r\n+\r\n-err\n");

    auto result = decoder.decode();
    auto ans = Frame{FrameID::SimpleString, "OK"};
    EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";

    result = decoder.decode();
    ans = Frame{FrameID::SimpleString, ""};
    EXPECT_EQ(result.value(), ans) << "can decode an empty simple string";

    result = decoder.decode();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid) << "Simple error frame is malformed, no CRLF";

    append_str(buffer, "$5\r\nhello\r\n-err\r\n:66\r\n:-5\r\n:0\r\n#t\r\n#f\r\n#n\r\n!3\r\nerr\r\n");

    result = decoder.decode();
    ans = Frame{FrameID::BulkString, "hello"};
    EXPECT_EQ(result.value(), ans) << "can decode a bulk string";

    result = decoder.decode();
    ans = Frame{FrameID::SimpleError, "err"};
    EXPECT_EQ(result.value(), ans) << "can decode a simple error";

    result = decoder.decode();
    ans = Frame{FrameID::Integer, 66};
    EXPECT_EQ(result.value(), ans) << "can decode a positive integer";

    result = decoder.decode();
    ans = Frame{FrameID::Integer, -5};
    EXPECT_EQ(result.value(), ans) << "can decode a negative integer";

    result = decoder.decode();
    ans = Frame{FrameID::Integer, 0};
    EXPECT_EQ(result.value(), ans) << "can decode a 0 as integer";

    result = decoder.decode();
    ans = Frame{FrameID::Boolean, true};
    EXPECT_EQ(result.value(), ans) << "can decode a boolean which is true";

    result = decoder.decode();
    ans = Frame{FrameID::Boolean, false};
    EXPECT_EQ(result.value(), ans) << "can decode a boolean which is false";

    result = decoder.decode();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid) << "can spot a malformed boolean frame";

    result = decoder.decode();
    ans = Frame{FrameID::BulkError, "err"};
    EXPECT_EQ(result.value(), ans) << "can decode a bulk error";
}

TEST_F(BufferManagerTest, DecodeSimpleArray)
{
    auto& buffer = decoder.get_buffer();
    append_str(buffer, "*3\r\n:1\r\n+Two\r\n$5\r\nThree\r\n");

    auto result = decoder.decode();
    auto vect = std::vector{Frame{FrameID::Integer, 1}, Frame{FrameID::SimpleString, "Two"},
                            Frame{FrameID::BulkString, "Three"}};
    auto ans = Frame{FrameID::Array, std::move(vect)};
    EXPECT_EQ(result.value(), ans) << "can decode a simple string with start a stream";
}
