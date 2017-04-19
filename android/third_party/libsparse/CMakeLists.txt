# The name of our project is "HELLO". CMakeLists files in this project can 
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and 
# to the root binary directory of the project as ${HELLO_BINARY_DIR}. 
cmake_minimum_required(VERSION 2.8 FATAL_ERROR) 
project(SPARSE)

# General include directories
include_directories(include SPARSE_INCLUDE_DIRS)
add_library(SPARSE
	src/backed_block.c
	src/output_file.c
	src/sparse.c
	src/sparse_crc32.c
	src/sparse_err.c
	src/sparse_read.c)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

