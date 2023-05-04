set(WINDOWS_ICON ../images/emulator_icon.rc)
android_add_executable(
  TARGET emulator-check INSTALL .
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${WINDOWS_ICON} main-emulator-check.cpp PlatformInfo.cpp
  DEPS android-emu android-emu-base-headers android-emu-crashreport
       libqemu2-glue)
target_include_directories(emulator-check PRIVATE ../..
                                                  ${ANDROID_QEMU2_TOP_DIR})

android_target_compile_options(emulator-check Clang
                               PRIVATE "-Wno-invalid-constexpr")
android_target_dependency(emulator-check all RUNTIME_OS_DEPENDENCIES)
android_target_properties(emulator-check all "${RUNTIME_OS_PROPERTIES}")

android_target_link_libraries(emulator-check linux-x86_64 PRIVATE -lX11)
android_target_link_libraries(emulator-check linux-aarch64 PRIVATE aemu-ui-qt
                                                                   -lXau)
