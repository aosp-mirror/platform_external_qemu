# Let's add in the library
android_add_library(
  TARGET avd-service-impl
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/incubating/AvdService.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-avd-grpc
       android-emu-base
       android-emu-feature
       android-emu-files
       android-grpc-utils
       android-grpc-event-waiter
       avd-service
       qemu-host-common-headers)
target_include_directories(avd-service-impl PUBLIC include PRIVATE src)

target_link_libraries(avd-service-impl PUBLIC avd-service grpc++)
