#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <openssl/md5.h>

std::string computeMD5(const std::string& str) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)str.c_str(), str.size(), (unsigned char*)&digest);

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        ss << std::hex << (int)digest[i];

    return ss.str();
}

std::string normalizeDef(const std::string& def) {
    std::stringstream ss(def);
    std::string line;
    std::string result;

    while (std::getline(ss, line, '\n')) {
        result += line + "\n";
    }

    return result;
}

std::pair<std::string, std::string> getStringMd5sum() {
    std::string string_msg_def = "string data";
    std::string string_md5sum = computeMD5(string_msg_def);
    return {string_msg_def, string_md5sum};
}

std::pair<std::string, std::string> getHeaderDef() {
    std::string header_msg_def = R"(
    uint32 seq
    time stamp
    string frame_id
    )";
    std::string normalized_header_message_def = normalizeDef(header_msg_def);
    std::string header_md5sum = computeMD5(normalized_header_message_def);
    return {normalized_header_message_def, header_md5sum};
}

std::pair<std::string, std::string> getImageMd5sum() {
    auto [header_def, header_md5sum] = getHeaderDef();
    std::string image_msg_def = header_md5sum + R"( header
    uint32 height
    uint32 width
    string encoding
    uint8 is_bigendian
    uint32 step
    uint8[] data
    )";
    std::string normalized_image_message_def = normalizeDef(image_msg_def);
    std::string image_md5sum = computeMD5(normalized_image_message_def);
    return {normalized_image_message_def, image_md5sum};
}

std::pair<std::string, std::string> getTemperatureDef() {
    auto [header_def, header_md5sum] = getHeaderDef();
    std::string tmp_msg_def = header_md5sum + R"( header
    float64 temperature
    float64 variance
    )";
    std::string normalized_tmp_message_def = normalizeDef(tmp_msg_def);
    std::string tmp_md5sum = computeMD5(normalized_tmp_message_def);
    return {normalized_tmp_message_def, tmp_md5sum};
}
