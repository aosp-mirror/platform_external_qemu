# Let's add in the library
android_add_library(
  TARGET modem-service-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/incubating/ModemService.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-feature
       android-emu-files
       android-emu-grpc-utils
       android-emu-telephony
       android-grpc-event-waiter
       modem_simulator_lib
       qemu-host-common-headers)
target_include_directories(modem-service-impl PUBLIC include PRIVATE src)

target_link_libraries(modem-service-impl PUBLIC modem-service)
