//
// Created by magnus on 10/16/23.
//

#ifndef ROSBAGS_CPP_TEST_HEADER_H
#define ROSBAGS_CPP_TEST_HEADER_H

#include <Header.h>
#include <gtest/gtest.h>

// Serialization tests
TEST(SerializationTests, TestSerializeUint8) {
auto result = serialize_uint8(42);
ASSERT_EQ(result.size(), 1);
ASSERT_EQ(result[0], 42);
}

TEST(SerializationTests, TestSerializeUint32) {
auto result = serialize_uint32(1234567890);
ASSERT_EQ(result.size(), 4);
ASSERT_EQ(result[0], 210);
ASSERT_EQ(result[1], 2);
ASSERT_EQ(result[2], 150);
ASSERT_EQ(result[3], 73);
}

TEST(SerializationTests, TestSerializeUint64) {
auto result = serialize_uint64(1234567890123456789);
ASSERT_EQ(result.size(), 8);
}

TEST(SerializationTests, TestSerializeTime) {
auto result = serialize_time(1234567890123456789);
ASSERT_EQ(result.size(), 8);
}

// Header class tests
TEST(HeaderTests, TestSetUint32) {
Header header;
header.set_uint32("test", 1234567890);
std::ostringstream stream;
header.write(stream);
// Validate output (add assertions as needed)
}

TEST(HeaderTests, TestSetUint64) {
Header header;
header.set_uint64("test", 1234567890123456789);
std::ostringstream stream;
header.write(stream);
// Validate output (add assertions as needed)
}

TEST(HeaderTests, TestSetString) {
Header header;
header.set_string("test", "value");
std::ostringstream stream;
header.write(stream);
// Validate output (add assertions as needed)
}

TEST(HeaderTests, TestSetTime) {
Header header;
header.set_time("test", 1234567890123456789);
std::ostringstream stream;
header.write(stream);
// Validate output (add assertions as needed)
}

TEST(HeaderTests, TestWrite) {
Header header;
header.set_string("key1", "value1");
header.set_uint32("key2", 1234567890);
std::ostringstream stream;
header.write(stream);
// Validate output (add assertions as needed)
}



#endif //ROSBAGS_CPP_TEST_HEADER_H
