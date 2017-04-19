include_directories(
  libselinux/include
  libsparse/include
  ext4_utils/include
  )

add_subdirectory(ext4_utils)
add_subdirectory(jpeg-6b)
add_subdirectory(libselinux)
add_subdirectory(libsparse)
add_subdirectory(libwebp)
add_subdirectory(mini-glib)

if(APPLE)
elseif(UNIX)
elseif(WIN32)
  add_subdirectory(mman-win32)
  add_subdirectory(regex-win32)
endif()


