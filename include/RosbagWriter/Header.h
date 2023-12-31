#ifndef ROSBAGWRITER_HEADER_H
#define ROSBAGWRITER_HEADER_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <climits>

namespace CRLRosWriter {

// Serialization helper functions
    static std::vector<uint8_t> serialize_uint8(uint8_t val) {
        return {val};
    }

    static std::vector<uint8_t> serialize_int32(int32_t val) {
        std::vector<uint8_t> bytes(4);
        for (int i = 0; i < 4; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }

    static std::vector<uint8_t> serialize_uint32(uint32_t val) {
        std::vector<uint8_t> bytes(4);
        for (int i = 0; i < 4; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }

    static std::vector<uint8_t> serialize_uint64(uint64_t val) {
        std::vector<uint8_t> bytes(8);
        for (int i = 0; i < 8; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }


    static std::vector<uint8_t> serialize_time(int64_t val) {
        auto sec = static_cast<int32_t>(val / 1000000000);
        auto nsec = static_cast<int32_t>(val % 1000000000);


        std::vector<uint8_t> bytes = serialize_int32(sec);
        std::vector<uint8_t> nsec_bytes = serialize_int32(nsec);  // Assuming you have a function to serialize int32_t
        bytes.insert(bytes.end(), nsec_bytes.begin(), nsec_bytes.end());
        return bytes;
    }

// Connection struct
    struct Connection {
        int id;
        std::string topic;
        std::string msgType;
        std::string md5sum;
        std::string msgDef;
        int msgcount;
        void *owner = nullptr;  // object type is translated to void pointer for simplicity
        Connection(int _id, std::string _topic, std::string _msgType, std::string _md5Sum,
                   std::string _msgDef, int _msgCount) : id(_id), topic(std::move(_topic)), msgType(std::move(_msgType)),
                                                              md5sum(std::move(_md5Sum)), msgDef(std::move(_msgDef)),
                                                              msgcount(_msgCount) {

        }

    };

// RecordType enum class
    enum class RecordType : int {
        MSGDATA = 2,
        BAGHEADER = 3,
        IDXDATA = 4,
        CHUNK = 5,
        CHUNK_INFO = 6,
        CONNECTION = 7,
        NONE = 0
    };

// Header class
    class Header {
    private:
        std::map<std::string, std::vector<uint8_t>> data;

    public:
        void set_uint32(const std::string &name, uint32_t value) {
            data[name] = serialize_uint32(value);
        }

        void set_uint64(const std::string &name, uint64_t value) {
            data[name] = serialize_uint64(value);
        }

        void set_string(const std::string &name, const std::string &value) {
            data[name] = std::vector<uint8_t>(value.begin(), value.end());
        }

        void set_time(const std::string &name, int64_t value) {
            data[name] = serialize_time(value);
        }

        int write(std::ostream &dst, RecordType opcode = RecordType::NONE) {
            std::vector<uint8_t> output;

            if (opcode != RecordType::NONE) {
                std::vector<uint8_t> key_bytes;
                std::vector<uint8_t> opcode_bytes = serialize_uint8(static_cast<uint8_t>(opcode));
                key_bytes.push_back('o');
                key_bytes.push_back('p');
                key_bytes.push_back('=');
                key_bytes.insert(key_bytes.end(), opcode_bytes.begin(), opcode_bytes.end());

                std::vector<uint8_t> size_bytes = serialize_uint32(key_bytes.size());
                output.insert(output.end(), size_bytes.begin(), size_bytes.end());
                output.insert(output.end(), key_bytes.begin(), key_bytes.end());
            }


            for (const auto &[key, value]: data) {
                std::vector<uint8_t> key_bytes(key.begin(), key.end());
                key_bytes.push_back('=');
                key_bytes.insert(key_bytes.end(), value.begin(), value.end());

                std::vector<uint8_t> size_bytes = serialize_uint32(key_bytes.size());
                output.insert(output.end(), size_bytes.begin(), size_bytes.end());
                output.insert(output.end(), key_bytes.begin(), key_bytes.end());
            }

            size_t size = output.size();
            auto size_s = serialize_uint32(size);
            output.insert(output.begin(), size_s.begin(), size_s.end());


            dst.write(reinterpret_cast<const char *>(output.data()), static_cast<uint32_t>(output.size()));
            return static_cast<int>(output.size());
        }
    };
}

#endif // ROSBAGWRITER_HEADER_H