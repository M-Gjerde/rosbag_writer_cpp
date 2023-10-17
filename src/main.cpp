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

#include <Header.h>
#include <utils.h>
#include "opencv4/opencv2/opencv.hpp"

struct WriteChunk {
    std::ostringstream data;
    int64_t pos;
    int64_t start;
    int64_t end;
    std::unordered_map<int, std::vector<std::pair<int64_t, int>>> connections;

    WriteChunk() : pos(-1), start(INT_MAX), end(0) {
        data = std::ostringstream(std::ios::binary);
    }
};

class RosbagWriter {
public:
    explicit RosbagWriter(const std::string &filename) : path(filename), chunk_threshold(20 * (1 << 20)) {
        chunks.emplace_back();
        bio = std::fstream(filename, std::ios::out | std::ios::binary);
        if (!bio) {
            std::cerr << "Error: Could not open file " << path << std::endl;
            std::cerr << "System error: " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    void open() {
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

    void write(Connection &connection, int64_t timestamp, std::vector<uint8_t> data) {
        WriteChunk &chunk = chunks.back();
        chunk.connections[connection.id].emplace_back(timestamp, static_cast<int>(chunk.data.tellp()));

        chunk.start = std::min(chunk.start, timestamp);
        chunk.end = std::max(chunk.end, timestamp);

        Header header;
        header.set_uint32("conn", connection.id);
        header.set_time("time", timestamp);

        header.write(chunk.data, RecordType::MSGDATA);
        chunk.data.write(reinterpret_cast<const char *>(serialize_uint32(data.size()).data()), 4);
        chunk.data.write(reinterpret_cast<const char *>(data.data()), data.size());

        if (chunk.data.tellp() > chunk_threshold) {
            write_chunk(chunk);
        }
    }

// Continuing within the RosbagWriter class from the previous conversions...


    void write_chunk(WriteChunk& chunk) {
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

            bio.write(reinterpret_cast<const char *>(serialize_uint32(data.size()).data()), 4);
            bio.write(data.c_str(), data.size());

            for (const auto& [cid, items] : chunk.connections) {
                Header idxHeader;
                idxHeader.set_uint32("ver", 1);
                idxHeader.set_uint32("conn", cid);
                idxHeader.set_uint32("count", items.size());
                idxHeader.write(bio, RecordType::IDXDATA);
                bio.write(reinterpret_cast<const char *>(serialize_uint32(items.size() * 12).data()), 4);

                for (const auto& [time, offset] : items) {
                    bio.write(reinterpret_cast<const char *>(serialize_time(time).data()), 8);
                    bio.write(reinterpret_cast<const char *>(serialize_uint32(offset).data()), 4);

                }
            }

            chunk.data.clear();
            chunks.emplace_back();
        }
    }


    Connection add_connection(const std::string &topic, const std::string &msg_type) {
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
        Connection connection(connections.size(), topic, msg_type, md5sum, msg_def, -1);
//
        auto &chunkBio = chunks.back().data;
        write_connection(connection, chunkBio);
        connections.push_back(connection);
        return connection;
    }

    void write_connection(const Connection &connection, std::ostream &bio) {
        Header header;
        header.set_uint32("conn", connection.id);
        header.set_string("topic", connection.topic);
        header.write(bio, RecordType::CONNECTION);

        Header msgHeader;
        msgHeader.set_string("topic", connection.topic);
        msgHeader.set_string("type", connection.msgtype);
        msgHeader.set_string("md5sum", connection.md5sum);
        msgHeader.set_string("message_definition", connection.msgdef);
        msgHeader.write(bio);
    }

    void close() {
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
            header.set_uint32("count", chunk.connections.size());
            header.write(bio, RecordType::CHUNK_INFO);

            int size = static_cast<int>(chunk.connections.size() * 8);
            bio.write(reinterpret_cast<const char *>(serialize_uint32(size).data()), 4);

            for (const auto &[cid, items]: chunk.connections) {
                bio.write(reinterpret_cast<const char *>(serialize_uint32(cid).data()), 4);
                bio.write(reinterpret_cast<const char *>(serialize_uint32(items.size()).data()), 4);
            }
        }

        bio.seekp(13);
        Header indexHeader;
        indexHeader.set_uint64("index_pos", index_pos);
        indexHeader.set_uint32("conn_count", connections.size());
        indexHeader.set_uint32("chunk_count", std::count_if(chunks.begin(), chunks.end(), [](const WriteChunk &chunk) {
            return chunk.pos != -1;
        }));
        int size = indexHeader.write(bio, RecordType::BAGHEADER);
        int padsize = 4096 - 4 - size;
        bio.write(reinterpret_cast<const char *>(serialize_uint32(padsize).data()), 4);
        bio.write(std::string(padsize, ' ').c_str(), padsize);
    }

    ~RosbagWriter() {
        close();

        if (bio.is_open()) {
            bio.close();
        }
    }

private:
    std::fstream bio;
    std::string path;
    std::vector<int> message_offsets;
    std::vector<Connection> connections;
    std::vector<WriteChunk> chunks;
    int chunk_threshold;
};


std::vector<uint8_t> header_dummy() {
    uint32_t seq = 1;
    std::vector<uint8_t> seq_serialized = serialize_uint32(seq);

    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch());
    int32_t secs = static_cast<int>(currentTimeNs.count() / 1'000'000'000);
    uint32_t nsecs = static_cast<int>(currentTimeNs.count() % 1'000'000'000);

    std::vector<uint8_t> stamp_secs = serialize_uint32(secs);
    std::vector<uint8_t> stamp_nsecs = serialize_uint32(nsecs);
    std::string str = "Magnus' Frame ID";
    std::vector<uint8_t> frame_id(str.begin(), str.end());
    std::vector<uint8_t> frame_id_length = serialize_uint32(frame_id.size());

    std::vector<uint8_t> output;
    output.insert(output.end(), seq_serialized.begin(), seq_serialized.end());
    output.insert(output.end(), stamp_secs.begin(), stamp_secs.end());
    output.insert(output.end(), stamp_nsecs.begin(), stamp_nsecs.end());
    output.insert(output.end(), frame_id_length.begin(), frame_id_length.end());
    output.insert(output.end(), frame_id.begin(), frame_id.end());


    return output;
}

std::vector<uint8_t> temperature_dummy() {
    std::vector<uint8_t> header = header_dummy();
    double temp = 24.0;
    double variance = 2.0;

    std::vector<uint8_t> temp_serialized = serialize_float64(temp);
    std::vector<uint8_t> var_serialized = serialize_float64(variance);
    std::vector<uint8_t> output;

    output.insert(output.end(), header.begin(), header.end());
    output.insert(output.end(), temp_serialized.begin(), temp_serialized.end());
    output.insert(output.end(), var_serialized.begin(), var_serialized.end());
    return output;
}


std::vector<uint8_t> image_dummy(uint32_t i) {
    std::vector<uint8_t> header = header_dummy();

    std::map<uint32_t, std::string> fileNames;

    fileNames[0] = "../one.jpg";
    fileNames[1] = "../two.jpg";
    fileNames[2] = "../three.jpg";
    std::string fileName = fileNames[i % 3];

    cv::Mat img = cv::imread(fileName, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Failed to load image: " << fileName << std::endl;
        // Handle the error accordingly...
        exit(1);
    }

    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    uint32_t height = img.rows;
    uint32_t width = img.cols;
    uint32_t step = width * 3;

    std::vector<uint8_t> widthSer = serialize_uint32(width);
    std::vector<uint8_t> heightSer = serialize_uint32(height);

    std::string str = "rgb8";
    std::vector<uint8_t> frame_id(str.begin(), str.end());
    std::vector<uint8_t> frame_id_length = serialize_uint32(frame_id.size());
    std::vector<uint8_t> bigEndian = serialize_uint8(0);
    std::vector<uint8_t> stepSer = serialize_uint32(step);

    // Serialize image
    std::vector<uint8_t> data;
    data.insert(data.end(), img.datastart, img.dataend);

    std::vector<uint8_t> dataLen = serialize_uint32(data.size());

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


int main() {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();


    RosbagWriter writer("../out.bag");
    writer.open();
    //auto conn2 = writer.add_connection("/hello_two", "std_msgs/String");

    //std::string str = "My man";
    //std::vector<uint8_t> d1(str.begin(), str.end());
//
    //std::string str2 = "My man Hello two again";
    //std::vector<uint8_t> d2(str2.begin(), str2.end());
//
    //writer.write(conn2, nanoseconds, d1);
    //writer.write(conn2,nanoseconds + int64_t(2), d2);
//
    //auto header_connection = writer.add_connection("/header", "std_msgs/Header");
    //writer.write(header_connection, nanoseconds + int64_t(1), header_dummy());

    auto image_connection = writer.add_connection("/images", "sensor_msgs/Image");

    for (int i = 0; i < 6; ++i){
        printf("Loading image: %d\n", i);
        int64_t time = nanoseconds + int64_t(i * 1e+9);

        writer.write(image_connection, time, image_dummy(i));
    }


    //auto header_conn = writer.add_connection("/temp", "sensor_msgs/Temperature");
    //writer.write(header_conn, nanoseconds + int64_t(9e8), temperature_dummy());

    return 0;
}