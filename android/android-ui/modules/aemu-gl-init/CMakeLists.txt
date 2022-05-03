android_qt_lib(
  TARGET aemu-gl-init
  # SHARED
  SRC android/main-emugl.cpp android/opengl/emugl_config.cpp
      android/opengl/EmuglBackendList.cpp android/opengl/EmuglBackendScanner.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-feature
       android-emu-files
       android-emu-utils
       android-hw-config
       qemu-host-common-headers
       ui::common)
# target_include_directories(aemu-gl-init PRIVATE .)
target_compile_options(aemu-gl-init PRIVATE -Wno-return-type-c-linkage)

# TODO(jansene): We should create X11 targets, as aarch64 needs the X11 libs
# from QT5 path
android_target_link_libraries(aemu-gl-init linux PRIVATE -lX11 ${QT5_LINK_PATH})

android_add_test(
  TARGET aemu-gl-init_unittests
  SOURCE_DIR test
  SRC android/opengl/emugl_config_unittest.cpp
      android/opengl/EmuglBackendList_unittest.cpp
      android/opengl/EmuglBackendScanner_unittest.cpp
  DEPS aemu-gl-init android-emu-base android-emu-base-headers
       android-emu-feature gmock_main qemu-host-common-headers)

target_include_directories(aemu-gl-init_unittests PRIVATE src)
