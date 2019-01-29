cmake_minimum_required(VERSION 3.5)
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

# We need the auto generated header for some components, so let's
# set the ANDROID_HW_CONFIG_H variable to point to the generated header.
# Those that need it can add it to their sources list, and it will be there.
android_generate_hw_config()
# Dependencies
prebuilt(E2FSPROGS)
prebuilt(LIBXML2)
prebuilt(UUID)

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
add_subdirectory(android/darwinn/external/proto/platforms/darwinn/model/config)
add_subdirectory(android/darwinn/proto)

# Grab the other libraries.
include(android-emu-base)
include(android-emu)
include(android-libui)
include(crash-service)

