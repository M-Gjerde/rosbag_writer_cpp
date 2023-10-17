#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>

namespace CRLRosWriter {

// Serialization helper functions
    std::vector<uint8_t> serialize_uint8(uint8_t val) {
        return {val};
    }

    std::vector<uint8_t> serialize_int32(int32_t val) {
        std::vector<uint8_t> bytes(4);
        for (int i = 0; i < 4; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }

    std::vector<uint8_t> serialize_uint32(uint32_t val) {
        std::vector<uint8_t> bytes(4);
        for (int i = 0; i < 4; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }

    std::vector<uint8_t> serialize_uint64(uint64_t val) {
        std::vector<uint8_t> bytes(8);
        for (int i = 0; i < 8; ++i) {
            bytes[i] = (val >> (i * 8)) & 0xFF;
        }
        return bytes;
    }

    std::vector<uint8_t> serialize_float64(double val) {
        static_assert(sizeof(double) * CHAR_BIT == 64, "64-bit double is assumed.");
        std::vector<uint8_t> bytes(8);
        auto *bytePtr = reinterpret_cast<uint8_t *>(&val);
        for (size_t i = 0; i < sizeof(double); ++i) {
            bytes[i] = bytePtr[i];
        }
        return bytes;
    }


    std::vector<uint8_t> serialize_time(int64_t val) {
        int32_t sec = static_cast<int32_t>(val / 1000000000);
        int32_t nsec = static_cast<int32_t>(val % 1000000000);


        std::vector<uint8_t> bytes = serialize_int32(sec);
        std::vector<uint8_t> nsec_bytes = serialize_int32(nsec);  // Assuming you have a function to serialize int32_t
        bytes.insert(bytes.end(), nsec_bytes.begin(), nsec_bytes.end());
        return bytes;
    }

// Connection struct
    struct Connection {
        int id;
        std::string topic;
        std::string msgtype;
        std::string md5sum;
        std::string msgdef;
        int msgcount;
        void *owner = nullptr;  // object type is translated to void pointer for simplicity
        Connection(int id, const std::string &topic, const std::string &msgtype, const std::string &md5Sum,
                   const std::string &msgdef, int msgcount) : id(id), topic(topic), msgtype(msgtype),
                                                              md5sum(md5Sum), msgdef(msgdef),
                                                              msgcount(msgcount) {

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


            dst.write(reinterpret_cast<const char *>(output.data()), output.size());
            return output.size();
        }
    };
};