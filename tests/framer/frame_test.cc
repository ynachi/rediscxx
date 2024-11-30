// Include necessary headers

#include "framer/frame.h"

#include <gtest/gtest.h>

// Assume the function and enum are part of a namespace or defined earlier

using namespace redis;
// Tests
TEST(FrameIDTest, Integer) { EXPECT_EQ(frame_id_from_char(kInteger), FrameID::Integer); }

TEST(FrameIDTest, SimpleString) { EXPECT_EQ(frame_id_from_char(kSimpleString), FrameID::SimpleString); }

TEST(FrameIDTest, SimpleError) { EXPECT_EQ(frame_id_from_char(kSimpleError), FrameID::SimpleError); }

TEST(FrameIDTest, BulkString) { EXPECT_EQ(frame_id_from_char(kBulkString), FrameID::BulkString); }

TEST(FrameIDTest, BulkError) { EXPECT_EQ(frame_id_from_char(kBulkError), FrameID::BulkError); }

TEST(FrameIDTest, Boolean) { EXPECT_EQ(frame_id_from_char(kBoolean), FrameID::Boolean); }

TEST(FrameIDTest, Null) { EXPECT_EQ(frame_id_from_char(kNull), FrameID::Null); }

TEST(FrameIDTest, BigNumber) { EXPECT_EQ(frame_id_from_char(kBigNumber), FrameID::BigNumber); }

TEST(FrameIDTest, Array) { EXPECT_EQ(frame_id_from_char(kArray), FrameID::Array); }

TEST(FrameIDTest, Undefined)
{
    EXPECT_EQ(frame_id_from_char('x'), FrameID::Undefined);  // Assuming 'x' is not mapped
}
