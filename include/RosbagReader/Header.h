//
// Created by mgjer on 28/11/2023.
//

#ifndef ROSBAG_WRITER_CPP_HEADER_H
#define ROSBAG_WRITER_CPP_HEADER_H

namespace RosbagReader {
    static uint8_t deserialize_uint8(const std::vector<uint8_t>& bytes, size_t& index) {
        return bytes[index++];
    }


    static int32_t deserialize_int32(const std::vector<uint8_t>& bytes, size_t& index) {
        int32_t val = 0;
        for (int i = 0; i < 4; ++i) {
            val |= (bytes[index++] << (i * 8));
        }
        return val;
    }

    static uint32_t deserialize_uint32(const std::vector<uint8_t>& bytes, size_t& index) {
        uint32_t val = 0;
        for (int i = 0; i < 4; ++i) {
            val |= (static_cast<uint32_t>(bytes[index++]) << (i * 8));
        }
        return val;
    }

    static uint64_t deserialize_uint64(const std::vector<uint8_t>& bytes, size_t& index) {
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val |= (static_cast<uint64_t>(bytes[index++]) << (i * 8));
        }
        return val;
    }


    static int64_t deserialize_time(const std::vector<uint8_t>& bytes, size_t& index) {
        int32_t sec = deserialize_int32(bytes, index);
        int32_t nsec = deserialize_int32(bytes, index);
        return static_cast<int64_t>(sec) * 1000000000 + nsec;
    }

}
#endif //ROSBAG_WRITER_CPP_HEADER_H
