#include <gtest/gtest.h>
#include "protocol.hh" // Include the header where Decoder is declared

using namespace redis;
class DecoderTest : public ::testing::Test {
protected:
    Decoder decoder;

    void SetUp() override {
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right before the destructor).
    }
};


TEST_F(DecoderTest, EmptyData) {
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(DecoderTest, IncompleteData) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("A", 1));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
}

TEST_F(DecoderTest, IncompleteCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
}

TEST_F(DecoderTest, InvalidCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF", 7));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid);
}

TEST_F(DecoderTest, ValidCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nCTL", 8));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_cursor_position(), 4);
}

TEST_F(DecoderTest, ValidCRLFBufferEnd) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\n", 5));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_cursor_position(), 0);
    EXPECT_EQ(decoder.get_buffer_number(), 0);
}

TEST_F(DecoderTest, DoubleValid) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nDEF\r\nHA", 12));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_cursor_position(), 4);
    EXPECT_EQ(decoder.get_buffer_number(), 1);

    result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "DEF");
}

// TEST_F(DecoderTest, ValidCRLFAcrossBuffers) {
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC", 3));
//     decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
//     auto result = decoder._read_until_crlf_simple();
//     ASSERT_TRUE(result.has_value());
//     EXPECT_EQ(result.value().first, 1); // Should point to the second buffer
//     EXPECT_EQ(result.value().second, 2); // Position in the second buffer
// }
