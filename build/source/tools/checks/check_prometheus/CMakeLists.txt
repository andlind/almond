cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0135 NEW)
project(check_prometheus)

# ==========================================
# 1. GLOBAL RPATH REMOVAL (Must be before FetchContent)
# ==========================================
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_SKIP_INSTALL_RPATH TRUE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN FALSE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)

# Optional but recommended for Nagios plugins: 
# Force static linking so CPR/cURL don't generate shared object (.so) dependencies at all.
# set(BUILD_SHARED_LIBS OFF)

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

# ==========================================
# 2. TARGET RPATH REMOVAL (Must be after add_executable)
# ==========================================
set_target_properties(check_prometheus PROPERTIES
    BUILD_RPATH ""
    INSTALL_RPATH ""
)

target_link_libraries(check_prometheus PRIVATE cpr::cpr nlohmann_json::nlohmann_json cxxopts::cxxopts)

# Assuming you install this via CMake for your RPM %install phase:
install(TARGETS check_prometheus DESTINATION /opt/almond/plugins)
