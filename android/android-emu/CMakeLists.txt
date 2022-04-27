cmake_minimum_required(VERSION 3.5)
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

# Dependencies
prebuilt(LIBXML2)
prebuilt(UUID)

# Add the protobuf dependencies
add_subdirectory(protos)

# Grab the other libraries.
include(android-emu)
if(NOT OPTION_GFXSTREAM_BACKEND)
  include(android-libui)
  include(crash-service)
endif()
