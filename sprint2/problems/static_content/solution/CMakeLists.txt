cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# Check if conanbuildinfo.cmake exists (Conan 1.x)
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    set(USE_CONAN TRUE)
else()
    set(USE_CONAN FALSE)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(USE_CONAN)
    # Using Conan
else()
    # Using system Boost
    find_package(Boost REQUIRED 
        COMPONENTS 
            system 
            filesystem
    )
endif()

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.h
	src/request_handler.cpp
)

if(USE_CONAN)
    target_link_libraries(game_server PRIVATE ${CONAN_LIBS} Threads::Threads)
else()
    target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(game_server PRIVATE 
        Threads::Threads
        ${Boost_LIBRARIES}
        -pthread
    )
endif()

