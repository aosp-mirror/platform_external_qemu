# Add extended controls service
android_add_library(
  TARGET ui-controller-server-impl
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/UiController.cpp
  DEPS android-emu android-emu-agents android-emu-base-headers grpc++
       qemu-host-common-headers)
target_include_directories(ui-controller-server-impl PUBLIC include PRIVATE src)

target_link_libraries(ui-controller-server-impl PUBLIC ui-controller-service)
