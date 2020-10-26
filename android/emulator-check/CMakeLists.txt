set(WINDOWS_ICON ../images/emulator_icon.rc)
android_add_executable(
  TARGET emulator-check INSTALL . LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${WINDOWS_ICON} main-emulator-check.cpp PlatformInfo.cpp)
target_include_directories(emulator-check PRIVATE ../..)

android_target_compile_options(emulator-check Clang
                               PRIVATE "-Wno-invalid-constexpr")
target_link_libraries(emulator-check PRIVATE android-emu)
android_target_dependency(emulator-check all RUNTIME_OS_DEPENDENCIES)
android_target_properties(emulator-check all "${RUNTIME_OS_PROPERTIES}")

android_target_link_libraries(emulator-check linux-x86_64 PRIVATE -lX11)
android_target_link_libraries(emulator-check linux-aarch64
                              PRIVATE emulator-libui)
