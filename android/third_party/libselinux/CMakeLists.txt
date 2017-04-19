# The name of our project is "HELLO". CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(SELINUX)

# General include directories
include_directories(include)
add_library(SELINUX
	src/callbacks.c
	src/check_context.c
	src/freecon.c
	src/init.c
	src/label.c
	src/label_file.c
	src/label_android_property.c)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

