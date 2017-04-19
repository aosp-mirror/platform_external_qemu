project(EXT4_UTILS)
list(APPEND EXT4_UTILS_SOURCES
  src/make_ext4fs.c
  src/ext4fixup.c
  src/ext4_utils.c
  src/allocate.c
  src/contents.c
  src/extent.c
  src/indirect.c
  src/uuid.c
  src/sha1.c
  src/wipe.c
  src/crc16.c
  src/ext4_sb.c
  )

if (WIN32)
  # (-lws2_32)
else()
  add_definitions(-DHOST)
endif()

include_directories(include)
add_library(EXT4_UTILS ${EXT4_UTILS_SOURCES})
add_executable(make_ext4fs src/make_ext4fs_main.c)
add_executable(ext2simg src/ext2simg.c)

target_link_libraries(EXT4_UTILS SPARSE)
target_link_libraries(make_ext4fs EXT4_UTILS SPARSE SELINUX z)
target_link_libraries(ext2simg EXT4_UTILS SPARSE SELINUX z)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

