android_add_library(
  TARGET android-emu-feature
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/cpu_accelerator.cpp
      android/emulation/CpuAccelerator.cpp
      android/featurecontrol/feature_control.cpp
      android/featurecontrol/FeatureControl.cpp
      android/featurecontrol/FeatureControlImpl.cpp
      android/featurecontrol/FeatureControlOverride.cpp
      android/featurecontrol/HWMatching.cpp
      android/HostHwInfo.cpp
      android/opengl/gpuinfo.cpp
  WINDOWS android/opengl/NativeGpuInfo_windows.cpp android/windows_installer.cpp
  LINUX android/opengl/NativeGpuInfo_linux.cpp
  DARWIN android/opengl/macTouchOpenGL.m android/opengl/NativeGpuInfo_darwin.cpp
  DEPS absl::strings
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-crashreport
       android-emu-curl
       android-emu-files
       android-emu-protos
       android-emu-studio-config
       android-emu-utils
       android-hw-config
       protobuf::libprotobuf)
target_include_directories(android-emu-feature PRIVATE src PUBLIC include)
android_target_link_libraries(android-emu-feature darwin
                              PUBLIC "-framework Metal")
target_compile_options(android-emu-feature PRIVATE "-Wno-extern-c-compat")

if(WINDOWS_MSVC_X86_64)
  # TODO(jansene): Remove once crash reporting is pulled out.
  target_include_directories(android-emu-feature PRIVATE ../../android-emu)
endif()
android_add_library(
  TARGET android-emu-feature-test
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR test
  SRC android/featurecontrol/testing/FeatureControlTest.cpp
  DEPS android-emu-base
       android-emu-base-headers
       android-emu-feature
       android-emu-files
       android-emu-protos
       android-emu-utils
       android-hw-config
       gtest
       protobuf::libprotobuf)
target_include_directories(android-emu-feature-test PUBLIC include src test)

android_add_test(
  TARGET android-emu-feature_unittests
  SOURCE_DIR test
  SRC android/emulation/CpuAccelerator_unittest.cpp
      android/featurecontrol/FeatureControl_unittest.cpp
      android/featurecontrol/HWMatching_unittest.cpp
      android/opengl/gpuinfo_unittest.cpp
  WINDOWS android/windows_installer_unittest.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-cmdline-testing
       android-emu-feature
       android-emu-feature-test
       android-emu-protos
       android-emu-test-launcher
       android-emu-utils
       protobuf::libprotobuf)
target_include_directories(android-emu-feature_unittests PRIVATE src test)
target_compile_options(android-emu-feature_unittests
                       PRIVATE "-Wno-extern-c-compat")

android_copy_file(
  android-emu-feature_unittests
  "${CMAKE_CURRENT_SOURCE_DIR}/test/android/emulation/CpuAccelerator_unittest.dat"
  "$<TARGET_FILE_DIR:android-emu-feature_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat"
)
android_copy_file(
  android-emu-feature_unittests
  "${CMAKE_CURRENT_SOURCE_DIR}/test/android/emulation/CpuAccelerator_unittest.dat2"
  "$<TARGET_FILE_DIR:android-emu-feature_unittests>/android/android-emu/android/emulation/CpuAccelerator_unittest.dat2"
)
