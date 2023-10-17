#include <filesystem>

#include <RosbagWriter/Header.h>
#include <RosbagWriter/utils.h>

namespace CRLRosWriter {

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
        explicit RosbagWriter(const std::filesystem::path &filename) : path(filename), chunk_threshold(20 * (1 << 20)) {
            chunks.emplace_back();
            bio = std::fstream(filename, std::ios::out | std::ios::binary);
            if (!bio) {
                std::cerr << "Error: Could not open file " << path << std::endl;
                std::cerr << "System error: " << strerror(errno) << std::endl;
                exit(1);
            }
        }

        Connection add_connection(const std::string &topic, const std::string &msg_type);
        void write(Connection &connection, int64_t timestamp, std::vector<uint8_t> data);


        void open();
        ~RosbagWriter() {
            close();

            if (bio.is_open()) {
                bio.close();
            }
        }

    private:
        std::fstream bio;
        std::filesystem::path path;
        std::vector<int> message_offsets;
        std::vector<Connection> connections;
        std::vector<WriteChunk> chunks;
        int chunk_threshold;


        void write_connection(const Connection &connection, std::ostream &bio);


        void write_chunk(WriteChunk &chunk);

        void close();
    };

};

/*
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


 */