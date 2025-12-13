cmake_minimum_required(VERSION 3.22)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/logging.cpp
    src/request_handler.cpp
    src/http_server.h
    src/logging.h
    src/request_handler.h
    src/util/token.h
    src/util/tagged.h
    src/model/dog.h
    src/model/player.h
    src/model/map.h
    src/model/position.h
    src/model/game_session.h
    src/application/game.h
    src/application/players.h
)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_include_directories(game_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(game_server PRIVATE CONAN_PKG::boost Threads::Threads)
