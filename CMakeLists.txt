cmake_minimum_required(VERSION 3.22)
project(rosbag_writer_cpp)

set(CMAKE_CXX_STANDARD 20)

# GTest
include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        DOWNLOAD_EXTRACT_TIMESTAMP YES
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
# Enable testing
enable_testing()

# Add directories
add_subdirectory(tests)

# Add the include directories for the test executable
add_executable(rosbags_cpp src/main.cpp)
target_include_directories(rosbags_cpp PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(rosbags_cpp -lssl -lcrypto)
