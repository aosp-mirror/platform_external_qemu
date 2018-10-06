cmake_minimum_required(VERSION 3.5)
# We can remove the if statement below once we no longer need the
# frankenstein building of make & cmake
if (NOT ANDROID_TARGET_TAG)
    get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_DIR}/../build/cmake" ABSOLUTE)
    list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
    include(frankenbuild)
endif()
include(android)
include(prebuilts)

# Make sure we can find our local defined targets
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

# Grabe the android-emu-base library
include(android-emu-base)
