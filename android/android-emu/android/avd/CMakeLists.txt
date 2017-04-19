list(APPEND SOURCES
  hw-config.c
  info.c
  scanner.c
  util.c
  #  util_unittest.cpp
  util_wrapper.cpp
  # util_wrapper_unittest.cpp 
  )

set(GENERATED_SOURCES hw-config-defs.h)

add_custom_command(
    OUTPUT ${GENERATED_SOURCES}
    POST_BUILD
    COMMAND ${QEMU}/android/scripts/gen-hw-config.py
    ${QEMU}/android/android-emu/android/avd/hardware-properties.ini hw-config-defs.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )


  add_library(avd ${SOURCES} ${GENERATED_SOURCES})


