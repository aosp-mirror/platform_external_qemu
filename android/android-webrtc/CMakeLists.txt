cmake_minimum_required(VERSION 3.5)
include(android)
if (WINDOWS_X86_64)
  message(FATAL_ERROR "MingW build is not supported.. Sorry.")
endif()


add_subdirectory(videobridge)
add_subdirectory(android-webrtc)
