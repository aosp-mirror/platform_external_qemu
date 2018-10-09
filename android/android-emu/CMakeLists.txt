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


# We need the auto generated header for some components, so let's
# set the ANDROID_HW_CONFIG_H variable to point to the generated header.
# Those that need it can add it to their sources list, and it will be there.
generate_hw_config()

prebuilt(PROTOBUF)
# Add the protobuf dependencies
add_subdirectory(android/metrics/proto)
add_subdirectory(android/featurecontrol/proto)
add_subdirectory(android/snapshot/proto)
add_subdirectory(android/crashreport/proto)
add_subdirectory(android/location/proto)
add_subdirectory(android/emulation/proto)
add_subdirectory(android/telephony/proto)
add_subdirectory(android/verified-boot/proto)
add_subdirectory(android/automation/proto)
add_subdirectory(android/offworld/proto)

# Grab the android-emu-base library
include(android-emu-base)
include(android-libui)

