# The name of our project is "HELLO". CMakeLists files in this project can 
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and 
# to the root binary directory of the project as ${HELLO_BINARY_DIR}. 
cmake_minimum_required(VERSION 2.8 FATAL_ERROR) 
project(MINI-GLIB)

if(WIN32)
	list(APPEND LOCAL_SRC src/glib-mini-win32.c)
endif()

list(APPEND LOCAL_SRC src/glib-mini.c)

# General include directories
include_directories(include)
add_library(MINI-GLIB ${LOCAL_SRC})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

