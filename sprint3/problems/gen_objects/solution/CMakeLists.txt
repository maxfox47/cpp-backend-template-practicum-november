cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_DISABLE_CHECK_COMPILER TRUE)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Устанавливаем выходную директорию для исполняемых файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(Model_Lib STATIC
	src/tagged.h
	src/model.h
	src/model.cpp
	src/loot_generator.h
	src/loot_generator.cpp
)

set(FILES
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/logger.h
	src/player.h
	src/player.cpp
	src/api_handler.h
	src/api_handler.cpp
	src/endpoint.h
	src/endpoint.cpp
)

add_executable(game_server
	src/main.cpp
	${FILES}
)

add_executable(model_tests
	tests/loot_generator_tests.cpp
)

target_include_directories(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE
 Threads::Threads
CONAN_PKG::boost
Model_Lib
)
target_link_libraries(model_tests PRIVATE CONAN_PKG::catch2 Model_Lib)

message(STATUS "Conan libraries: ${CONAN_LIBS}")
