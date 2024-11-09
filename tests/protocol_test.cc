#include "protocol.hh"
#include <gtest/gtest.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/seastar.hh>

using namespace redis;
class BufferManagerTest : public ::testing::Test {
protected:
    BufferManager decoder;

    void SetUp() override {
        // Code here will be called immediately after the constructor (right before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right before the destructor).
    }
};


TEST_F(BufferManagerTest, GetSimpleString_EmptyData) {
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetSimpleString_IncompleteData) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("A", 1));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
    EXPECT_EQ(decoder.get_total_size(), 1);
}

TEST_F(BufferManagerTest, GetSimpleString_IncompleteCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
}

TEST_F(BufferManagerTest, GetSimpleString_InvalidLFOnly) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF", 7));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid);
    EXPECT_EQ(decoder.get_current_buffer_size(), 3); // only DEF should be left
}

TEST_F(BufferManagerTest, GetSimpleString_InvalidCROnly) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\rDEF", 7));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid);
    EXPECT_EQ(decoder.get_current_buffer_size(), 3); // only DEF should be left
}

TEST_F(BufferManagerTest, GetSimpleString_ValidCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nCTL", 8));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_current_buffer_size(), 3);
}

TEST_F(BufferManagerTest, GetSimpleString_ValidCRLFBufferEnd) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\n", 5));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_buffer_number(), 0);
}

TEST_F(BufferManagerTest, GetSimpleString_DoubleValid) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nDEF\r\nHA", 12));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_buffer_number(), 1);

    result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "DEF");
}

TEST_F(BufferManagerTest, GetSimpleString_ValidCRLFAcrossBuffers) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC", 3));
    decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
    auto result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_buffer_number(), 1);
}

TEST_F(BufferManagerTest, GetSimpleString_BadThenGood) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF\r\nHA", 11));
    auto result = decoder.get_simple_string();
    EXPECT_EQ(result.error(), FrameDecodeError::Invalid);
    result = decoder.get_simple_string();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "DEF");
    EXPECT_EQ(decoder.get_current_buffer_size(), 2);
}

TEST_F(BufferManagerTest, Advance) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF\r\nHA", 12));
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC", 3));
    decoder.advance(3);
    EXPECT_EQ(decoder.get_current_buffer_size(), 9);
    EXPECT_EQ(decoder.get_buffer_number(), 2);
    decoder.advance(9);
    EXPECT_EQ(decoder.get_current_buffer_size(), 3);
    EXPECT_EQ(decoder.get_buffer_number(), 1);
}

// ****************
// get_bulk_string
// **************
TEST_F(BufferManagerTest, GetBulkString_EmptyData) {
    auto result = decoder.get_bulk_string(6);
    EXPECT_EQ(result.error(), FrameDecodeError::Empty);
}

TEST_F(BufferManagerTest, GetBulkString_IncompleteData) {
    // @TODO: we should return an error in this case, because the frame is well formed by
    // @TODO the actual data is not enough to decode the frame.
    decoder.add_upstream_data(seastar::temporary_buffer<char>("A\r\n", 3));
    auto result = decoder.get_bulk_string(2);
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
}

TEST_F(BufferManagerTest, GetBulkString_IncompleteCRLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
    auto result = decoder.get_bulk_string(3);
    EXPECT_EQ(result.error(), FrameDecodeError::Incomplete);
    EXPECT_EQ(decoder.get_total_size(), 4);
}

TEST_F(BufferManagerTest, GetBulkString_CanDecodeSimpleString) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r\nDEF", 8));
    auto result = decoder.get_bulk_string(3);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(decoder.get_current_buffer_size(), 3); // DEF left
}

TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthLF) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\nDEF\r\nA", 10));
    auto result = decoder.get_bulk_string(7);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "ABC\nDEF");
    EXPECT_EQ(decoder.get_current_buffer_size(), 1); // DEF left
}

TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthCR) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\rDEF\r\nA", 10));
    auto result = decoder.get_bulk_string(7);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "ABC\rDEF");
    EXPECT_EQ(decoder.get_current_buffer_size(), 1); // DEF left
}

TEST_F(BufferManagerTest, GetBulkString_CanDecodeStringWIthCRLatest) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\rDEF\r\n", 9));
    auto result = decoder.get_bulk_string(7);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "ABC\rDEF");
    ASSERT_EQ(decoder.get_buffer_number(), 0);
    EXPECT_EQ(decoder.get_current_buffer_size(), 0); // DEF left
}

TEST_F(BufferManagerTest, GetBulkString_SimpleValidCRLFAcrossBuffers) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC", 3));
    decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
    auto result = decoder.get_bulk_string(3);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC");
    EXPECT_EQ(decoder.get_buffer_number(), 1);
}

TEST_F(BufferManagerTest, GetBulkString_BulkValidCRLFAcrossBuffers) {
    decoder.add_upstream_data(seastar::temporary_buffer<char>("ABC\r", 4));
    decoder.add_upstream_data(seastar::temporary_buffer<char>("\r\nDEF", 5));
    auto result = decoder.get_bulk_string(4);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "ABC\r");
    EXPECT_EQ(decoder.get_buffer_number(), 1);
}

int main(int argc, char **argv) {
    seastar::app_template app;
    return app.run(argc, argv, [&argc, &argv] {
        ::testing::InitGoogleTest(&argc, argv);
        return seastar::make_ready_future<int>(RUN_ALL_TESTS());
    });
}
