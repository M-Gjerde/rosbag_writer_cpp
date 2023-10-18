#ifndef ROSBAGWRITER_UTILS_H
#define ROSBAGWRITER_UTILS_H


#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <openssl/md5.h>
#include <openssl/evp.h>

namespace CRLRosWriter {

    static std::string computeMD5(const std::string &content) {
        EVP_MD_CTX *context = EVP_MD_CTX_new();
        const EVP_MD *md = EVP_md5();
        unsigned char md_value[EVP_MAX_MD_SIZE];
        unsigned int md_len;
        std::string output;

        EVP_DigestInit_ex(context, md, nullptr);
        EVP_DigestUpdate(context, content.c_str(), content.length());
        EVP_DigestFinal_ex(context, md_value, &md_len);
        EVP_MD_CTX_free(context);

        output.resize(md_len * 2);
        for (unsigned int i = 0; i < md_len; ++i)
            std::sprintf(&output[i * 2], "%02x", md_value[i]);
        return output;
    }

    static std::string normalizeDef(const std::string &def) {
        std::stringstream ss(def);
        std::stringstream result;
        std::string line;
        std::string previousLine;

        while (std::getline(ss, line, '\n')) {
            // Trim whitespace from the start and end.
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());

            // Remove comments.
            auto comment_pos = line.find("#");
            if (comment_pos != std::string::npos) {
                line.erase(comment_pos);
            }

            // Skip empty lines.
            if (!line.empty()) {
                if (!previousLine.empty()) {
                    result << previousLine << "\n";
                }
                previousLine = line;
            }
        }

        // Add the last line without appending a newline
        if (!previousLine.empty()) {
            result << previousLine;
        }

        return result.str();
    }

    static std::pair<std::string, std::string> getStringMd5sum() {
        std::string string_msg_def = "string data";
        std::string string_md5sum = computeMD5(string_msg_def);
        return {string_msg_def, string_md5sum};
    }

    static std::pair<std::string, std::string> getHeaderDef() {
        std::string header_msg_def = "uint32 seq\n"
                                     "time stamp\n"
                                     "string frame_id";

        std::string normalized_header_message_def = normalizeDef(header_msg_def);
        std::string header_generated_md5sum = computeMD5(header_msg_def);
        return {normalized_header_message_def, header_generated_md5sum};
    }

    static std::pair<std::string, std::string> getImageMd5sum() {
        auto [header_def, header_md5sum] = getHeaderDef();
        std::string image_msg_def = header_md5sum + " header\n"
                                                    "uint32 height\n"
                                                    "uint32 width\n"
                                                    "string encoding\n"
                                                    "uint8 is_bigendian\n"
                                                    "uint32 step\n"
                                                    "uint8[] data";

        std::string normalized_image_message_def = normalizeDef(image_msg_def);
        std::string image_md5sum = computeMD5(normalized_image_message_def);
        return {normalized_image_message_def, image_md5sum};
    }


    static std::pair<std::string, std::string> getTemperatureDef() {
        auto [header_def, header_md5sum] = getHeaderDef();
        std::string tmp_msg_def = header_md5sum + R"( header
    float64 temperature
    float64 variance
    )";
        std::string normalized_tmp_message_def = normalizeDef(tmp_msg_def);
        std::string tmp_md5sum = computeMD5(normalized_tmp_message_def);
        return {normalized_tmp_message_def, tmp_md5sum};
    }


};

#endif // ROSBAGWRITER_UTILS_H