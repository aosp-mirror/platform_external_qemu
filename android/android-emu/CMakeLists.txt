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
prebuilt(PNG)
prebuilt(UUID)


# Grab the other libraries
include(android-emu)
include(android-libui)
include(crash-service)

