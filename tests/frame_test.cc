//
// Created by ynachi on 9/10/24.
//

#include <frame.hh>
#include <gtest/gtest.h>

using namespace redis;

struct TestCase
{
    Frame input;
    std::string expected_ans;
};

// Test data
std::vector<TestCase> test_cases = {
        {Frame{FrameID::SimpleString, "ABC"}, "+ABC\r\n"},
        {Frame{FrameID::SimpleString, ""}, "+\r\n"},
        {Frame{FrameID::SimpleError, "ABC"}, "-ABC\r\n"},
        {Frame{FrameID::SimpleError, ""}, "-\r\n"},
        {Frame{FrameID::Integer, 0}, ":0\r\n"},
        {Frame{FrameID::Integer, 33}, ":33\r\n"},
        {Frame{FrameID::Integer, -234}, ":-234\r\n"},
        {Frame{FrameID::BulkString, "ABC"}, "$3\r\nABC\r\n"},
        {Frame{FrameID::BulkString, ""}, "$0\r\n\r\n"},
        {Frame{FrameID::BulkString, "AB\rC"}, "$4\r\nAB\rC\r\n"},
        {Frame{FrameID::BulkString, "AB\nC"}, "$4\r\nAB\nC\r\n"},
        {Frame{FrameID::BulkError, "ABC"}, "!3\r\nABC\r\n"},
        {Frame{FrameID::BulkError, ""}, "!0\r\n\r\n"},
        {Frame{FrameID::BulkError, "AB\rC"}, "!4\r\nAB\rC\r\n"},
        {Frame{FrameID::BulkError, "AB\nC"}, "!4\r\nAB\nC\r\n"},
        {Frame{FrameID::Boolean, true}, "#t\r\n"},
        {Frame{FrameID::Boolean, false}, "#f\r\n"},
        {Frame{FrameID::Null, false}, "_\r\n"},
        {Frame{FrameID::Array,
               std::vector<Frame>{Frame{FrameID::BulkString, "hello"}, Frame{FrameID::BulkString, "world"}}},
         "*2\r\n$5\r\nhello\r\n$5\r\nworld\r\n"},
};

// Test fixture for parameterized tests
class TableBasedTest : public ::testing::TestWithParam<TestCase>
{
};

// Define the parameterized test
TEST_P(TableBasedTest, TOString)
{
    // Get the current test case
    auto [input, expected_ans] = GetParam();

    // Call the function with the test case inputs
    const std::string result = input.to_string();

    // Assert that the result matches the expected output
    EXPECT_EQ(result, expected_ans);
}

// Instantiate the test suite with the test cases
INSTANTIATE_TEST_SUITE_P(TestingTableBased,  // Name of the test suite
                         TableBasedTest,  // Test fixture class
                         ::testing::ValuesIn(test_cases)  // Range of values to test
);
