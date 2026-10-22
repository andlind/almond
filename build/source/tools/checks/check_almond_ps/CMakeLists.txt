cmake_minimum_required(VERSION 3.10)

# Name your project
project(check_almond_ps)

# Require C++11 standard for std::regex, std::stod, etc.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the libcurl package installed on your system
find_package(CURL REQUIRED)

# Add your source file to the executable
add_executable(check_almond_ps main.cpp)

# Link libcurl to your executable
target_link_libraries(check_almond_ps CURL::libcurl)
