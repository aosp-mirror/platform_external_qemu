
set(WINDOWS_ICON ../images/emulator_icon.rc)
add_executable(emulator-check main-emulator-check.cpp PlatformInfo.cpp ${WINDOWS_ICON})
target_include_directories(emulator-check PRIVATE ../..)

android_target_compile_options(emulator-check Clang PRIVATE "-Wno-invalid-constexpr")
target_link_libraries(emulator-check PRIVATE android-emu)

# The mingw debug build somehow requires us to link against the vm operations in debug builds, even though
# we are not using them..
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  android_target_link_libraries(emulator-check windows-x86_64 PRIVATE android-mock-vm-operations android-emu)
endif()


android_target_dependency(emulator-check all  RUNTIME_OS_DEPENDENCIES)
android_target_properties(emulator-check all "${RUNTIME_OS_PROPERTIES}")

android_target_link_libraries(emulator-check linux-x86_64 PRIVATE -lX11)
android_install_exe(emulator-check .)

