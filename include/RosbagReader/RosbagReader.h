//
// Created by mgjer on 28/11/2023.
//

#ifndef ROSBAG_WRITER_CPP_ROSBAGREADER_H
#define ROSBAG_WRITER_CPP_ROSBAGREADER_H


#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "Header.h"

class RosbagReader {
    std::fstream bio;

public:
    void open(const std::filesystem::path &filePath);
    void readHeader();
    void readData();
    void deserializeMessage(const std::vector<uint8_t>& message);
    // Additional methods to handle specific types of data, e.g., images or IMU data
};



#endif //ROSBAG_WRITER_CPP_ROSBAGREADER_H
