# The name of our project is "HELLO". CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(REGEX-WIN32)

if(NOT WIN32)
	MESSAGE(FATAL_ERROR "This is a windows only library")
endif()

# General include directories
include_directories(include compat)
add_library(REGEX-WIN32
	compat/reallocarr.c
	compat/strlcpy.c
	src/regcomp.c
	src/regerror.c
	src/regexec.c
	src/regfree.c )

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

