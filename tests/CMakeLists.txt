# Add test executable
add_executable(test_main
        src/test_main.cpp
        src/Test_Header.cpp
        # Add other test files as your test suite grows
)

target_include_directories(test_main PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link with GTest libraries
target_link_libraries(test_main   GTest::gtest_main -lssl -lcrypto rosbag_cpp_writer
)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(test_main)
