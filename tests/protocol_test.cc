#include "protocol.hh"
#include <gtest/gtest.h>

using namespace redis;
class BufferManagerTest : public ::testing::Test {
protected:
    ProtocolDecoder decoder;

    void SetUp() override {
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right before the destructor).
    }
};

void append_str(std::vector<char>& vec, const char* str) {
    vec.insert(vec.end(), str, str + std::strlen(str));
}

TEST_F(BufferManagerTest, GetSimpleString_EmptyData) {
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetSimpleString_IncompleteData) {
    auto &buffer = decoder.get_buffer();
    buffer.push_back('A');
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete) <<"frame should be terminated by CRLF";
    EXPECT_EQ(buffer.size(), 1) << R"(buffer should not be altered when hitting "incomplete" error type)";

     append_str(buffer, "BC\r"); // os is now "ABC\r"
     auto result2 = decoder.get_simple_string();
     EXPECT_EQ(result2.error(), FrameDecodeError::Incomplete) << "single CR at the end probably means incomplete frame";
     EXPECT_EQ(buffer.size(), 4) << R"(buffer should not be altered when hitting "incomplete" error type)";

     append_str(buffer, "DEF"); // os is now "ABC\rDEF"
     auto result3 = decoder.get_simple_string();
     EXPECT_EQ(result3.error(), FrameDecodeError::Invalid) << "isolated CR in a simple frame is an error";
     EXPECT_EQ(buffer.size(), 3) << R"(should strip out "ABC\r" and let "DEF")";

    append_str(buffer, "\nDEF"); // os is now "DEF\nDEF"
    auto result4 = decoder.get_simple_string();
    EXPECT_EQ(result4.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
    EXPECT_EQ(buffer.size(), 3) << R"(should strip out "DEF\n" and let "DEF")";

     append_str(buffer, "\nABC\r\nDEF\r\n55\r\n"); // os is now "DEF\nABC\r\nDEF\r\n55\r\n"
     auto result5 = decoder.get_simple_string();
     EXPECT_EQ(result5.error(), FrameDecodeError::Invalid) << "isolated LF in a simple frame is an error";
     EXPECT_EQ(buffer.size(), 14) << R"(should strip out "DEF\n")";

     auto result6 = decoder.get_simple_string();
     EXPECT_EQ(result6.value(), "ABC") << "can decode a valid frame";
     EXPECT_EQ(buffer.size(), 9) << R"(should strip out "ABC\r\n", notice CRLF is included)";

     auto result7 = decoder.get_simple_string();
     EXPECT_EQ(result7.value(), "DEF") << "can decode a valid frame after another in the same stream";
     EXPECT_EQ(buffer.size(), 4) << R"(should strip out "DEF\r\n", notice CRLF is included)";
     std::cout << buffer.size() << std::endl;

     auto result8 = decoder.get_simple_string();
     EXPECT_EQ(result8.value(), "55") << "can decode a valid frame at the end of a stream";
     EXPECT_EQ(buffer.size(), 0) << R"(should strip out "55\r\n", notice CRLF is included)";
}

// ****************
// get_bulk_string
// **************
TEST_F(BufferManagerTest, GetBulkString_EmptyData) {
    auto result = decoder.get_bulk_string(6);
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetBulkString_IncompleteData) {
    auto &buffer = decoder.get_buffer();
    buffer.push_back('A');
    buffer.push_back('\r');
    buffer.push_back('\n');
    // @TODO: fix this to return an invalid error instead
    auto result = decoder.get_bulk_string(2);
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);

    const auto result1 = decoder.get_bulk_string(1);
    EXPECT_EQ(result1.value(), "A") <<"get_bulk_string: can decode a simple 1 byte frame";
    EXPECT_EQ(buffer.size(), 0) << R"(get_bulk_string: should consume the character and CRLF)";
}

// TEST_F(BufferManagerTest, GetBulkString_IncompleteCRLF) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
//     auto result = decoder.get_bulk_string(3);
//     EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
//     EXPECT_EQ(decoder.get_total_size(), 4);
// }
//
// TEST_F(BufferManagerTest, GetBulkString_CanDecodeSimpleString) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nDEF", 8));
//     auto result = decoder.get_bulk_string(3);
//     ASSERT_TRUE(result.has_value());
//     EXPECT_EQ(decoder.get_current_buffer_size(), 3); // DEF left
// }
//
// TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthLF) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF\r\nA", 10));
//     auto result = decoder.get_bulk_string(7);
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result.value(), "ABC\nDEF");
//     EXPECT_EQ(decoder.get_current_buffer_size(), 1); // DEF left
// }
//
// TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthCR) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\rDEF\r\nA", 10));
//     auto result = decoder.get_bulk_string(7);
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result.value(), "ABC\rDEF");
//     EXPECT_EQ(decoder.get_current_buffer_size(), 1); // DEF left
// }
//
// TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthCRLatest) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\rDEF\r\n", 9));
//     auto result = decoder.get_bulk_string(7);
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result.value(), "ABC\rDEF");
//     ASSERT_EQ(decoder.get_buffer_number(), 0);
//     EXPECT_EQ(decoder.get_current_buffer_size(), 0); // DEF left
// }
//
// TEST_F(BufferManagerTest, GetBulkString_SimpleValidCRLFAcrossBuffers) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC", 3));
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
//     auto result = decoder.get_bulk_string(3);
//     ASSERT_TRUE(result.has_value());
//     EXPECT_EQ(result.value(), "ABC");
//     EXPECT_EQ(decoder.get_buffer_number(), 1);
// }
//
// TEST_F(BufferManagerTest, GetBulkString_BulkValidCRLFAcrossBuffers) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
//     auto result = decoder.get_bulk_string(4);
//     ASSERT_TRUE(result.has_value());
//     EXPECT_EQ(result.value(), "ABC\r");
//     EXPECT_EQ(decoder.get_buffer_number(), 1);
// }
