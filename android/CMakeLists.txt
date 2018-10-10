cmake_minimum_required(VERSION 3.5)
# We can remove the if statement below once we no longer need the frankenstein
# building of make & cmake
if(NOT ANDROID_TARGET_TAG)
  get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_DIR}/build/cmake"
                         ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(frankenbuild)
endif()
include(android)
include(prebuilts)

# In our frankenbuild this is passed in, we can remove this later.
if(NOT ANDROID_FRANKENBUILD)
  if(EMULATOR_CRASHUPLOAD)
    set(ANDROID_EMULATOR_CRASHUPLOAD ${EMULATOR_CRASHUPLOAD})
  else()
    set(ANDROID_EMULATOR_CRASHUPLOAD "NONE")
  endif()
  # Make sure we don't use exceptions
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# We want to make sure we binplace everything in the root, so that gnumake can
# pick up all our created artifacts
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Let's not go nuts with reporting.
set(CMAKE_RULE_MESSAGES OFF)
set(CMAKE_TARGET_MESSAGES OFF)

add_subdirectory(third_party)
add_subdirectory(android-emu)
