cmake_minimum_required(VERSION 2.8.12)
project(lox)

set(VERSION "0.0.1")

set (
	CMAKE_BINARY_DIR
	"${CMAKE_SOURCE_DIR}/build"
)

set (
	EXECUTABLE_OUTPUT_PATH
	"${CMAKE_SOURCE_DIR}/build/bin"
)

set (
	LIBRARY_OUTPUT_PATH
	"${CMAKE_SOURCE_DIR}/build/bin"
)

set (
	PROJECT_SOURCE_DIR
	"src/"
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

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${CMAKE_BINARY_DIR}")
add_executable(lox "${PROJECT_SOURCE_DIR}/main.c")
