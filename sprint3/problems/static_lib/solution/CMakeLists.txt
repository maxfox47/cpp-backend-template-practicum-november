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

# Статическая библиотека игровой модели
# Содержит только игровую логику без зависимостей от сетевого кода
add_library(model_lib STATIC
	src/tagged.h
	src/loot_generator.h
	src/loot_generator.cpp
	src/model.h
	src/model.cpp
)

# Устанавливаем публичные заголовки библиотеки
# Это позволяет другим целям использовать заголовки через target_link_libraries
target_include_directories(model_lib
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:include>
)

# Библиотека модели не требует внешних зависимостей (использует только стандартную библиотеку C++)
# Поэтому не нужно указывать зависимости как PUBLIC

# Файлы инфраструктурного слоя (HTTP сервер, обработчики запросов, JSON загрузчик)
# Эти файлы не входят в библиотеку модели, так как они не тестируются
# и содержат зависимости от сетевого кода
set(INFRASTRUCTURE_FILES
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

# Исполняемый файл игрового сервера
# Использует библиотеку модели и инфраструктурный код
add_executable(game_server
	src/main.cpp
	${INFRASTRUCTURE_FILES}
)

target_include_directories(game_server 
	PRIVATE 
		Threads::Threads 
		CONAN_PKG::boost
)

target_link_libraries(game_server 
	PRIVATE
		Threads::Threads
		CONAN_PKG::boost
		model_lib
)

# Исполняемый файл для тестов
# Тестирует только библиотеку модели
add_executable(model_tests
	tests/loot_generator_tests.cpp
)

target_link_libraries(model_tests 
	PRIVATE 
		CONAN_PKG::catch2 
		model_lib
)

# Интеграция Catch2 с CTest
# Позволяет использовать команду ctest для запуска тестов
include(CTest)
if(DEFINED CONAN_BUILD_DIRS_CATCH2 AND EXISTS "${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake")
	include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
	catch_discover_tests(model_tests)
elseif(DEFINED CONAN_BUILD_DIRS_CATCH2_DEBUG AND EXISTS "${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake")
	include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
	catch_discover_tests(model_tests)
endif()

message(STATUS "Conan libraries: ${CONAN_LIBS}")

