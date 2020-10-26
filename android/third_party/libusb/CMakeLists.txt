cmake_minimum_required(VERSION 3.5)

set(src "${ANDROID_QEMU2_TOP_DIR}/../libusb")

android_add_library(
  TARGET emulator-libusb
  LIBNAME libusb
  LICENSE LGPL-2.1
  SRC ${src}/libusb/core.c ${src}/libusb/descriptor.c ${src}/libusb/hotplug.c
      ${src}/libusb/io.c ${src}/libusb/strerror.c ${src}/libusb/sync.c
  DARWIN ${src}/libusb/os/darwin_usb.c ${src}/libusb/os/poll_posix.c
         ${src}/libusb/os/threads_posix.c
  LINUX ${src}/libusb/os/linux_netlink.c ${src}/libusb/os/linux_usbfs.c
        ${src}/libusb/os/poll_posix.c ${src}/libusb/os/threads_posix.c
  WINDOWS ${src}/libusb/os/poll_windows.c ${src}/libusb/os/threads_windows.c
          ${src}/libusb/os/windows_common.c ${src}/libusb/os/windows_usbdk.c
          ${src}/libusb/os/windows_winusb.c)

target_include_directories(emulator-libusb PUBLIC ${src}/libusb)

# Windows config
android_target_include_directories(emulator-libusb windows PRIVATE ${src}/msvc)
android_target_compile_definitions(emulator-libusb windows PRIVATE
                                   "-Dstatic_assert=_Static_assert")

# Linux configuration
android_target_include_directories(emulator-libusb linux-x86_64 PRIVATE linux)
android_target_link_libraries(emulator-libusb linux-x86_64
                              PRIVATE Threads::Threads)

android_target_include_directories(emulator-libusb linux-aarch64 PRIVATE linux)
android_target_link_libraries(emulator-libusb linux-aarch64
                              PRIVATE Threads::Threads)

# Darwin
android_target_include_directories(emulator-libusb darwin-x86_64 PRIVATE
                                   ${src}/Xcode)
android_target_link_libraries(
  emulator-libusb darwin-x86_64 PRIVATE "-framework IOKit"
                                        "-framework CoreFoundation")

# These are very slow.. 27.97s user 90.95s system 75% cpu 2:37.73 total
android_add_executable(TARGET libusb_stress_tests NODISTRIBUTE
                       SRC ${src}/tests/stress.c ${src}/tests/testlib.c)
target_link_libraries(libusb_stress_tests PRIVATE emulator-libusb)
