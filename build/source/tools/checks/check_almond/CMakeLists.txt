cmake_minimum_required(VERSION 3.14)
project(check_almond)

set(CMAKE_CXX_STANDARD 17)

# Find the required libraries on your system
find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)

add_executable(check_almond main.cpp)

# Link the libraries to your target
target_link_libraries(check_almond  PRIVATE CURL::libcurl jsoncpp_lib)
