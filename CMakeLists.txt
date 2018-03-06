cmake_minimum_required(VERSION 2.8.12)
project(lox)

set(VERSION "0.0.1")

set (
	CMAKE_BINARY_DIR
	"${CMAKE_SOURCE_DIR}/build"
)

set (
	EXECUTABLE_OUTPUT_PATH
	"${CMAKE_SOURCE_DIR}/bin"
)

set (
	LIBRARY_OUTPUT_PATH
	"${CMAKE_SOURCE_DIR}/bin"
)

if (CMAKE_BUILD_TYPE EQUAL "DEBUG" OR NOT DEFINED(CMAKE_BUILD_TYPE))
	set (
		DEBUG
		"DEBUG"
	)
endif()

configure_file (
	"lox-config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/includes/lox-config.h"
	@ONLY
)

include_directories ("${PROJECT_SOURCE_DIR}/include/")
include_directories ("${PROJECT_SOURCE_DIR}/include/ds")
include_directories ("${CMAKE_BINARY_DIR}")
file(GLOB_RECURSE LOX_SRC
	"${PROJECT_SOURCE_DIR}/include/*.h"
	"${PROJECT_SOURCE_DIR}/src/*.c"
)

if(MSVC)
	ADD_DEFINITIONS(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /Za")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -ansi")
endif()

message("-- Compiling with ${CMAKE_CXX_FLAGS}")
add_executable(lox ${LOX_SRC})
if(WIN32)
else()
	target_link_libraries(lox m)
endif()