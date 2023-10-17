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

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Add directories
add_subdirectory(tests)

# Add the include directories for the test executable
add_library(rosbag_cpp_writer src/RosbagWriter.h)
target_include_directories(rosbag_cpp_writer PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(rosbag_cpp_writer PROPERTIES LINKER_LANGUAGE CXX)


if (UNIX) ## Linux
    target_link_libraries(rosbag_cpp_writer -lssl -lcrypto )
endif ()

if (WIN32)
    target_link_libraries(rosbag_cpp_writer OpenSSL::SSL OpenSSL::Crypto)
endif ()
