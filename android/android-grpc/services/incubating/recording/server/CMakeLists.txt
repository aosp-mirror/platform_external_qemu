# Let's add in the library
android_add_library(
  TARGET screen-recording-service-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/incubating/ScreenRecordingService.cpp
  DEPS aemu::recording
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-files
       android-grpc-utils
       android-grpc-event-waiter
       android-hw-config
       qemu-host-common-headers)
target_include_directories(screen-recording-service-impl PUBLIC include
                           PRIVATE src)

target_link_libraries(screen-recording-service-impl
                      PUBLIC screen-recording-service)
