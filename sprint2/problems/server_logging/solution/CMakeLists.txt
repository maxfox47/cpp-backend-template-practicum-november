cmake_minimum_required(VERSION 3.22)
project(server_logging CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

find_package(Threads REQUIRED)

add_executable(server_logging
    src/main.cpp
    src/http_server.cpp
    src/logging.cpp
    src/request_handler.cpp
    src/http_server.h
    src/logging.h
    src/request_handler.h
)

target_include_directories(server_logging PRIVATE CONAN_PKG::boost)

target_link_libraries(server_logging PRIVATE CONAN_PKG::boost Threads::Threads)


