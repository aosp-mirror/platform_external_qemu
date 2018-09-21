set(WINDOWS_ICON ../images/emulator_icon.rc)
add_executable(emulator-check
               main-emulator-check.cpp
               PlatformInfo.cpp
               ${WINDOWS_ICON})
target_include_directories(emulator-check PRIVATE ../..)
target_compile_options(emulator-check PRIVATE "-Wno-invalid-constexpr")
target_link_libraries(emulator-check PRIVATE android-emu emulator-libui)
target_prebuilt_dependency(emulator-check "${RUNTIME_OS_DEPENDENCIES}"
                           "${RUNTIME_OS_PROPERTIES}")

if(APPLE OR ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
  # target_compile_options(emulator-check PRIVATE -fno-strict-aliasing -fno-
  # common) target_link_libraries(emulator-check PRIVATE -lbz2 "-framework
  # VideoToolbox" "-framework Accelerate" "-framework VideoDecodeAcceleration"
  # libOpenGLESDispatch )
elseif(WIN32 OR ANDROID_TARGET_TAG MATCHES "windows.*")

elseif(UNIX OR ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  target_link_libraries(emulator-check PRIVATE -lX11)
endif()
