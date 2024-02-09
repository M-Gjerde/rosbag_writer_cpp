cmake_minimum_required(VERSION 3.14)
project(rosbag_writer_cpp)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_TESTS "Build tests for rosbag_writer_cpp" ON)
set(BUILD_AS_VIEWER_DEPENDENCY ON)
include(cmake/CompilerWarnings.cmake)


if (${BUILD_AS_VIEWER_DEPENDENCY})
    add_library(libsll_static STATIC IMPORTED GLOBAL)
    set_target_properties(libsll_static PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/openssl_1.1.1/lib/libssl.lib")

    add_library(libcrypto_static STATIC IMPORTED GLOBAL)
    set_target_properties(libcrypto_static PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/openssl_1.1.1/lib/libcrypto.lib")
else()
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (BUILD_TESTS)
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
endif ()


# Add the include directories for the test executable
add_library(rosbag_cpp_writer src/RosbagWriter.cpp)
target_include_directories(rosbag_cpp_writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(rosbag_cpp_writer PROPERTIES LINKER_LANGUAGE CXX)
set_project_warnings(rosbag_cpp_writer)


if (UNIX) ## Linux
    target_link_libraries(rosbag_cpp_writer -lssl -lcrypto)
endif ()

if (WIN32)
    target_link_libraries(rosbag_cpp_writer libsll_static libcrypto_static)
    target_include_directories(rosbag_cpp_writer PUBLIC "${CMAKE_SOURCE_DIR}/external/openssl_1.1.1/include")
endif ()
