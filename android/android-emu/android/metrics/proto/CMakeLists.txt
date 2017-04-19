cmake_minimum_required (VERSION 2.8 FATAL_ERROR) 
project (proto)

if (NOT WIN32) 
	add_subdirectory(${EXT4_UTILS_SOURCE_DIR}/../libselinux libselinux)
	include_directories(${SELINUX_INCLUDE_DIRS})
endif()

add_subdirectory(${EXT4_UTILS_SOURCE_DIR}/../libsparse libsparse)
include_directories(include ${SPARSE_INCLUDE_DIRS})
include_directories(include)
add_library(EXT4_UTILS  src/make_ext4fs.c src/ext4fixup.c src/ext4_utils.c  src/allocate.c src/contents.c src/extent.c src/indirect.c src/uuid.c src/sha1.c src/wipe.c src/crc16.c src/ext4_sb.c)
target_link_libraries(EXT4_UTILS ../libsparse)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
