//
// Created by magnus on 10/16/23.
//

#ifndef ROSBAGS_CPP_TEST_HEADER_H
#define ROSBAGS_CPP_TEST_HEADER_H

#include <RosbagWriter/Header.h>
#include <gtest/gtest.h>
#include <cstring>

// Serialization tests
TEST(SerializationTests, TestSerializeUint8) {
    auto result = CRLRosWriter::serialize_uint8(42);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0], 42);
}

TEST(SerializationTests, TestSerializeUint32) {
    auto result = CRLRosWriter::serialize_uint32(1234567890);
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 210);
    ASSERT_EQ(result[1], 2);
    ASSERT_EQ(result[2], 150);
    ASSERT_EQ(result[3], 73);
}

TEST(SerializationTests, TestSerializeUint64) {
    auto result = CRLRosWriter::serialize_uint64(1234567890123456789);
    ASSERT_EQ(result.size(), 8);
}

TEST(SerializationTests, TestSerializeTime) {
    auto result = CRLRosWriter::serialize_time(1234567890123456789);
    ASSERT_EQ(result.size(), 8);
}

// Header class tests
TEST(HeaderTests, TestSetUint32) {
    CRLRosWriter::Header header;
    header.set_uint32("test", 1234567890);
    std::ostringstream stream;
    header.write(stream);
    std::string output = stream.str();

    // The exact validation depends on how the `Header` class writes the uint32 value.
    // For this example, let's assume it writes it in a human-readable format:
    ASSERT_NE(output.find("test"), std::string::npos) << "Key 'test' not found in the output";
    // Extract the serialized uint32 value from the output.
    // The exact way to do this depends on the structure of the binary data.
    // For this example, let's assume the uint32 value starts at byte 4 and is 4 bytes long.


    auto pos = output.find("test");
    const char* serialized_uint32 = &output[pos + 5];

    // Expected little-endian representation of 1234567890
    char expected_value[4];
    expected_value[0] = 0xD2;
    expected_value[1] = 0x02;
    expected_value[2] = 0x96;
    expected_value[3] = 0x49;

    // Validate that the serialized uint32 value matches the expected value
    ASSERT_EQ(0, std::memcmp(serialized_uint32, expected_value, 4))
                                << "Serialized uint32 value does not match expected value";
}

TEST(HeaderTests, TestSetUint64) {
    CRLRosWriter::Header header;
    header.set_uint64("test", 1234567890123456789);
    std::ostringstream stream;
    header.write(stream);
    std::string result = stream.str();
    // Assuming the Header class writes the key followed by the value
    ASSERT_TRUE(result.find("test") != std::string::npos);
    ASSERT_TRUE(result.find("\x15\x81\xE9\x7D\xF4\x10\x22\x11") != std::string::npos);  // 1234567890123456789 in little-endian
}

TEST(HeaderTests, TestSetString) {
    CRLRosWriter::Header header;
    header.set_string("test", "value");
    std::ostringstream stream;
    header.write(stream);
    std::string result = stream.str();
    // Assuming the Header class writes the key followed by the value
    ASSERT_TRUE(result.find("test") != std::string::npos);
    ASSERT_TRUE(result.find("value") != std::string::npos);
}

TEST(HeaderTests, TestSetTime) {
    CRLRosWriter::Header header;
    header.set_time("test", 1234567890123456789);
    std::ostringstream stream;
    header.write(stream);
    std::string result = stream.str();
    // Assuming the Header class writes the key followed by the value
    ASSERT_TRUE(result.find("test") != std::string::npos);
    ASSERT_TRUE(result.find("\xD2\x02\x96\x49\x00\x00\x00\x00") != std::string::npos);  // 1234567890123456789 in little-endian
}

TEST(HeaderTests, TestWrite) {
    CRLRosWriter::Header header;
    header.set_string("key1", "value1");
    header.set_uint32("key2", 1234567890);
    std::ostringstream stream;
    header.write(stream);
    std::string result = stream.str();
    // Assuming the Header class writes the keys and values in the order they were set
    ASSERT_TRUE(result.find("key1") != std::string::npos);
    ASSERT_TRUE(result.find("value1") != std::string::npos);
    ASSERT_TRUE(result.find("key2") != std::string::npos);
    ASSERT_TRUE(result.find("\xD2\x02\x96\x49") != std::string::npos);  // 1234567890 in little-endian


}



#endif //ROSBAGS_CPP_TEST_HEADER_H
