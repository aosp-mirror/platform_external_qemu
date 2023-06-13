set(studio-bridge_src
    ${EMULATOR_GRPC_SRC}
    android/emulation/control/EmulatorService.cpp
    android/emulation/control/keyboard/KeyEventSender.cpp
    android/emulation/control/GrpcServices.cpp
    android/emulation/control/waterfall/BstrStreambuf.cpp
    android/emulation/control/waterfall/SocketStreambuf.cpp
    android/emulation/control/waterfall/WaterfallConnection.cpp
    android/emulation/control/waterfall/WaterfallService.cpp)

# Let's add in the library
android_add_library(
  TARGET studio-bridge SHARED LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                     ${studio-bridge_src})
target_link_libraries(studio-bridge PRIVATE android-emu android-emu-base-streams)

# Lets distribute the proto file
install(studio-bridge DESTINATION lib)
