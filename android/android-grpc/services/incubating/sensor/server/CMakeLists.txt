# Let's add in the library
android_add_library(
  TARGET sensor-service-impl
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/incubating/SensorService.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-base
       android-emu-feature
       android-grpc-utils
       android-grpc-event-waiter
       qemu-host-common-headers
       sensor-service)

target_include_directories(sensor-service-impl PUBLIC include PRIVATE src)
target_link_libraries(sensor-service-impl PUBLIC sensor-service grpc++)
