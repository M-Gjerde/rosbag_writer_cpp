//
// Created by magnus on 10/17/23.
//
#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include <cmath>
#include <climits>
#include <cerrno>
#include <cstring>

#include <RosbagWriter/RosbagWriter.h>

namespace CRLRosWriter {

    void RosbagWriter::open(const std::filesystem::path &filePath) {
        if (opened)
            return;
        path = filePath;

        bio = std::fstream(path.string(), std::ios::out | std::ios::binary);
        if (!bio) {
            std::cerr << "Error: Could not open file " << path << std::endl;
            exit(1);
        }

        writeHeader();
        opened = true;
    }

    void RosbagWriter::writeHeader() {
        bio.write("#ROSBAG V2.0\n", 13);
        Header header;
        header.set_uint64("index_pos", 0);
        header.set_uint32("conn_count", 1);
        header.set_uint32("chunk_count", 1);
        int size = header.write(bio, RecordType::BAGHEADER);
        int padsize = 4096 - 4 - size;
        bio.write(reinterpret_cast<const char *>(serialize_uint32(padsize).data()), 4);
        bio.write(std::string(padsize, ' ').c_str(), padsize);
    }

    void RosbagWriter::write(Connection &connection, int64_t timestamp, std::vector<uint8_t> data) {
        WriteChunk &chunk = chunks.back();
        chunk.connections[connection.id].emplace_back(timestamp, static_cast<int>(chunk.data.tellp()));

        chunk.start = std::min(chunk.start, timestamp);
        chunk.end = std::max(chunk.end, timestamp);

        Header header;
        header.set_uint32("conn", connection.id);
        header.set_time("time", timestamp);

        header.write(chunk.data, RecordType::MSGDATA);
        chunk.data.write(reinterpret_cast<const char *>(serialize_uint32(static_cast<uint32_t>(data.size())).data()), 4);
        chunk.data.write(reinterpret_cast<const char *>(data.data()), static_cast<uint32_t>(data.size()));

        if (chunk.data.tellp() > chunk_threshold) {
            write_chunk(chunk);
        }
    }

// Continuing within the RosbagWriter class from the previous conversions...


    void RosbagWriter::write_chunk(WriteChunk &chunk) {
        if (!bio.is_open()) {
            std::cerr << "File not open!" << std::endl;
            return;
        }

        int size = static_cast<int>(chunk.data.tellp());
        if (size > 0) {
            chunk.pos = static_cast<int>(bio.tellp());

            Header header;
            header.set_string("compression", "none");
            header.set_uint32("size", size);
            header.write(bio, RecordType::CHUNK);

            std::string data = chunk.data.str();

            bio.write(reinterpret_cast<const char *>(serialize_uint32(static_cast<uint32_t>(data.size())).data()), 4);
            bio.write(data.c_str(), static_cast<uint32_t>(data.size()));

            for (const auto &[cid, items]: chunk.connections) {
                Header idxHeader;
                idxHeader.set_uint32("ver", 1);
                idxHeader.set_uint32("conn", cid);
                idxHeader.set_uint32("count", static_cast<uint32_t>(items.size()));
                idxHeader.write(bio, RecordType::IDXDATA);
                bio.write(reinterpret_cast<const char *>(serialize_uint32(static_cast<uint32_t>(items.size() * 12)).data()), 4);

                for (const auto &[time, offset]: items) {
                    bio.write(reinterpret_cast<const char *>(serialize_time(time).data()), 8);
                    bio.write(reinterpret_cast<const char *>(serialize_uint32(offset).data()), 4);

                }
            }

            chunk.data.clear();
            chunks.emplace_back();
        }
    }


    Connection RosbagWriter::add_connection(const std::string &topic, const std::string &msg_type) {

        std::string msg_def, md5sum;

        if (msg_type.find("Image") != std::string::npos) {
            tie(msg_def, md5sum) = getImageMd5sum();
        } else if (msg_type.find("Header") != std::string::npos) {
            tie(msg_def, md5sum) = getHeaderDef();
        } else if (msg_type.find("Temperature") != std::string::npos) {
            tie(msg_def, md5sum) = getTemperatureDef();
        } else {
            tie(msg_def, md5sum) = getStringMd5sum();
        }

        std::cout << "md5: " << md5sum << " for " << msg_type << std::endl;
        Connection connection(static_cast<int>(connections.size()), topic, msg_type, md5sum, msg_def, -1);
//
        auto &chunkBio = chunks.back().data;
        write_connection(connection, chunkBio);
        connections.push_back(connection);
        return connection;
    }

    void RosbagWriter::write_connection(const Connection &connection, std::ostream &bagIO) {
        Header header;
        header.set_uint32("conn", connection.id);
        header.set_string("topic", connection.topic);
        header.write(bagIO, RecordType::CONNECTION);

        Header msgHeader;
        msgHeader.set_string("topic", connection.topic);
        msgHeader.set_string("type", connection.msgType);
        msgHeader.set_string("md5sum", connection.md5sum);
        msgHeader.set_string("message_definition", connection.msgDef);
        msgHeader.write(bagIO);
    }

    void RosbagWriter::close() {
        std::cout << "Closing" << std::endl;
        if (!bio.is_open()) return;

        for (WriteChunk &chunk: chunks) {
            if (chunk.pos == -1) {
                write_chunk(chunk);
            }
        }

        int index_pos = static_cast<int>(bio.tellp());

        for (const Connection &connection: connections) {
            write_connection(connection, bio);
        }

        for (const WriteChunk &chunk: chunks) {
            if (chunk.pos == -1) continue;

            Header header;
            header.set_uint32("ver", 1);
            header.set_uint64("chunk_pos", chunk.pos);
            header.set_time("start_time", chunk.start == INT_MAX ? 0 : chunk.start);
            header.set_time("end_time", chunk.end);
            header.set_uint32("count", static_cast<uint32_t>(chunk.connections.size()));
            header.write(bio, RecordType::CHUNK_INFO);

            int size = static_cast<int>(chunk.connections.size() * 8);
            bio.write(reinterpret_cast<const char *>(serialize_uint32(size).data()), 4);

            for (const auto &[cid, items]: chunk.connections) {
                bio.write(reinterpret_cast<const char *>(serialize_uint32(cid).data()), 4);
                bio.write(reinterpret_cast<const char *>(serialize_uint32(static_cast<uint32_t>(items.size())).data()), 4);
            }
        }

        bio.seekp(13);
        Header indexHeader;
        indexHeader.set_uint64("index_pos", index_pos);
        indexHeader.set_uint32("conn_count", static_cast<uint32_t>(connections.size()));
        indexHeader.set_uint32("chunk_count",static_cast<uint32_t>(
                               std::count_if(chunks.begin(), chunks.end(), [](const WriteChunk &chunk) {
                                   return chunk.pos != -1;
                               })));
        int size = indexHeader.write(bio, RecordType::BAGHEADER);
        int padsize = 4096 - 4 - size;
        bio.write(reinterpret_cast<const char *>(serialize_uint32(padsize).data()), 4);
        bio.write(std::string(padsize, ' ').c_str(), padsize);
        opened = false;
    }


    Connection RosbagWriter::getConnection(const std::string &topic, const std::string &msgType){
        for (const auto &conn: connections) {
            if (conn.topic == topic && conn.msgType == msgType)
                return conn;
        }
        return add_connection(topic, msgType);
    };

    std::vector<uint8_t> RosbagWriter::serializerRosHeader(uint32_t sequence, int64_t currentTimeNs) {
        std::vector<uint8_t> seq_serialized = serialize_uint32(sequence);

        auto secs = static_cast<int>(currentTimeNs / 1'000'000'000);
        uint32_t nsecs = static_cast<int>(currentTimeNs % 1'000'000'000);

        std::vector<uint8_t> stamp_secs = serialize_uint32(secs);
        std::vector<uint8_t> stamp_nsecs = serialize_uint32(nsecs);
        std::string str = "Header frame";
        std::vector<uint8_t> frame_id(str.begin(), str.end());
        std::vector<uint8_t> frame_id_length = serialize_uint32(static_cast<uint32_t>(frame_id.size()));

        std::vector<uint8_t> output;
        output.insert(output.end(), seq_serialized.begin(), seq_serialized.end());
        output.insert(output.end(), stamp_secs.begin(), stamp_secs.end());
        output.insert(output.end(), stamp_nsecs.begin(), stamp_nsecs.end());
        output.insert(output.end(), frame_id_length.begin(), frame_id_length.end());
        output.insert(output.end(), frame_id.begin(), frame_id.end());
        return output;
    }


    std::vector<uint8_t>
    RosbagWriter::serializeImage(uint32_t sequence, int64_t timestamp, uint32_t width, uint32_t height, uint8_t *pData, uint32_t dataSize,
                                 const std::string &encoding, uint32_t stepSize) {
        std::vector<uint8_t> header = serializerRosHeader(sequence, timestamp);


        uint32_t step = stepSize;

        std::vector<uint8_t> widthSer = serialize_uint32(width);
        std::vector<uint8_t> heightSer = serialize_uint32(height);

        std::string str = encoding;
        std::vector<uint8_t> frame_id(str.begin(), str.end());
        std::vector<uint8_t> frame_id_length = serialize_uint32(static_cast<uint32_t>(frame_id.size()));
        std::vector<uint8_t> bigEndian = serialize_uint8(0);
        std::vector<uint8_t> stepSer = serialize_uint32(step);

        // Serialize image
        std::vector<uint8_t> data;
        data.insert(data.end(), pData, pData + dataSize);

        std::vector<uint8_t> dataLen = serialize_uint32(static_cast<uint32_t>(data.size()));

        std::vector<uint8_t> output;

        output.insert(output.end(), header.begin(), header.end());
        output.insert(output.end(), heightSer.begin(), heightSer.end());
        output.insert(output.end(), widthSer.begin(), widthSer.end());
        output.insert(output.end(), frame_id_length.begin(), frame_id_length.end());
        output.insert(output.end(), frame_id.begin(), frame_id.end());
        output.insert(output.end(), bigEndian.begin(), bigEndian.end());
        output.insert(output.end(), stepSer.begin(), stepSer.end());
        output.insert(output.end(), dataLen.begin(), dataLen.end());
        output.insert(output.end(), data.begin(), data.end());

        return output;

    }
}

