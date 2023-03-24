# Let's add in the library
android_add_library(
  TARGET car-service-impl
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/incubating/CarService.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-feature
       android-emu-grpc-utils
       android-grpc-event-waiter
       car-service
       qemu-host-common-headers)
target_include_directories(car-service-impl PUBLIC include PRIVATE src)

target_link_libraries(car-service-impl PUBLIC car-service grpc++)
