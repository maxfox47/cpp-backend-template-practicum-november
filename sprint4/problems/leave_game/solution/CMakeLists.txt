cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_DISABLE_CHECK_COMPILER TRUE)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED COMPONENTS serialization wserialization)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(model_lib STATIC
	src/tagged.h
	src/model.h
	src/model.cpp
	src/loot_generator.h
	src/loot_generator.cpp
)

add_library(collision_detection_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/tagged.h
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
	src/serialization.h
	src/serialization.cpp
	src/state_saver.h
	src/model_serialization.h
)

add_executable(model_tests
tests/loot-generator-tests.cpp
)

add_executable(collision_detection_tests
tests/collision-detector-tests.cpp
)

add_executable(state_serialization_tests
tests/state-serialization-tests.cpp
src/model_serialization.h
)

target_include_directories(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE
 Threads::Threads
CONAN_PKG::boost
model_lib
collision_detection_lib
)
target_link_libraries(model_tests Threads::Threads CONAN_PKG::catch2 model_lib)
target_link_libraries(state_serialization_tests Threads::Threads CONAN_PKG::catch2 model_lib Boost::serialization 
    Boost::wserialization)
target_link_libraries(collision_detection_tests Threads::Threads CONAN_PKG::catch2 collision_detection_lib)

message(STATUS "Conan libraries: ${CONAN_LIBS}")
