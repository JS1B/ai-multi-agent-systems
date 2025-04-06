cmake_minimum_required(VERSION 3.10)
project(searchclient_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add executable
add_executable(searchclient 
    src/main.cpp
)

# Add test executable
add_executable(searchclient_tests
    src/tests/color_tests.cpp
    src/tests/action_tests.cpp
)

# Add include directories
target_include_directories(searchclient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(searchclient_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link test executable with gtest
target_link_libraries(searchclient_tests
    GTest::gtest_main
)

# Add test command
enable_testing()
add_test(NAME searchclient_tests COMMAND searchclient_tests)
