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

struct WriteChunk {
    std::ostringstream data;
    int pos;
    int start;
    int end;
    std::unordered_map<int, std::vector<std::pair<int, int>>> connections;

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

    void write(Connection &connection, int timestamp, const std::string &data) {
        WriteChunk &chunk = chunks.back();
        chunk.connections[connection.id].push_back(std::make_pair(timestamp, static_cast<int>(chunk.data.tellp())));

        chunk.start = std::min(chunk.start, timestamp);
        chunk.end = std::max(chunk.end, timestamp);

        Header header;
        header.set_uint32("conn", connection.id);
        header.set_time("time", timestamp);

        header.write(chunk.data, RecordType::MSGDATA);
        chunk.data.write(reinterpret_cast<const char *>(serialize_uint32(data.size()).data()), 4);
        chunk.data.write(data.c_str(), data.size());

        if (chunk.data.tellp() > chunk_threshold) {
            //write_chunk(chunk);
        }
    }

// Continuing within the RosbagWriter class from the previous conversions...

/*
    void write_chunk(WriteChunk& chunk) {
        if (!bio.is_open()) {
            std::cerr << "File not open!" << std::endl;
            return;
        }

        int size = static_cast<int>(chunk.data.tell());
        if (size > 0) {
            chunk.pos = static_cast<int>(bio.tellp());

            Header header;
            header.set_string("compression", "none");
            header.set_uint32("size", size);
            header.write(bio, RecordType::CHUNK);

            std::string data = chunk.data.str();
            bio.write(serialize_uint32(data.size()).c_str(), 4);
            bio.write(data.c_str(), data.size());

            for (const auto& [cid, items] : chunk.connections) {
                Header idxHeader;
                idxHeader.set_uint32("ver", 1);
                idxHeader.set_uint32("conn", cid);
                idxHeader.set_uint32("count", items.size());
                idxHeader.write(bio, RecordType::IDXDATA);
                bio.write(serialize_uint32(items.size() * 12).c_str(), 4);

                for (const auto& [time, offset] : items) {
                    bio.write((serialize_time(time) + serialize_uint32(offset)).c_str(), 16);
                }
            }

            chunk.data.clear();
            chunks.push_back(WriteChunk());
        }
    }
    */

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
        if (!bio.is_open()) return;

        for (WriteChunk &chunk: chunks) {
            if (chunk.pos == -1) {
                //write_chunk(chunk);
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

/*
std::string header_dummy() {
    int seq = 1;
    std::string seq_serialized = serialize_uint32(seq);

    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch());
    int secs = static_cast<int>(currentTimeNs.count() / 1'000'000'000);
    int nsecs = static_cast<int>(currentTimeNs.count() % 1'000'000'000);

    std::string stamp_secs = serialize_int32(secs);
    std::string stamp_nsecs = serialize_uint32(nsecs);
    std::string frame_id = "Magnus' Frame ID";
    std::string frame_id_length = serialize_uint32(frame_id.size());

    return seq_serialized + stamp_secs + stamp_nsecs + frame_id_length + frame_id;
}

std::string temperature_dummy() {
    std::string header = header_dummy();
    double temp = 24.0;
    double variance = 2.0;

    std::string temp_serialized = serialize_double(temp);
    std::string var_serialized = serialize_double(variance);

    return header + temp_serialized + var_serialized;
}
*/
int main() {
    auto currentTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    /*
    auto header_conn = writer.add_connection("/temp", "sensor_msgs/Temperature");
    writer.write(header_conn, currentTimeNs, temperature_dummy());
    */
    RosbagWriter writer("out.bag");
    writer.open();
    auto conn2 = writer.add_connection("/hello_two", "std_msgs/String");
    writer.write(conn2, currentTimeNs, "Hello two");
    writer.write(conn2, currentTimeNs + 1, "Hello two again");
    return 0;
}