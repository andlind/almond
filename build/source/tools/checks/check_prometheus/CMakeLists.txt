cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0135 NEW)
project(check_prometheus)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.4
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

add_executable(check_prometheus main.cpp)
target_link_libraries(check_prometheus PRIVATE cpr::cpr nlohmann_json::nlohmann_json cxxopts::cxxopts)
