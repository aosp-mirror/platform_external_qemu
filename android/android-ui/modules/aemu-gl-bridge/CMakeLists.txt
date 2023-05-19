android_qt_lib(
  TARGET aemu-gl-bridge
  ALIAS ui::gl-bridge
  SRC android/gpu_frame.cpp android/skin/qt/gl-canvas.cpp
      android/skin/qt/gl-texture-draw.cpp
  DEPS android-emu android-emu-base android-emu-base-headers android-emu-utils
       emugl_common qemu-host-common-headers)
target_include_directories(aemu-gl-bridge PUBLIC src)
target_include_directories(aemu-gl-bridge PRIVATE .)
target_compile_options(aemu-gl-bridge PRIVATE -Wno-return-type-c-linkage)
android_target_link_libraries(aemu-gl-bridge linux-x86_64 PRIVATE -lX11)

android_qt_lib(
  TARGET aemu-gl-qt-bridge
  ALIAS ui::gl-qt-bridge
  SRC android/skin/qt/gl-common.cpp android/skin/qt/gl-widget.cpp
      android/skin/qt/gl-widget.h
  DEPS android-emu
       android-emu-base
       android-emu-base-headers
       android-emu-utils
       emugl_common
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-bridge)
target_include_directories(aemu-gl-qt-bridge PUBLIC src)
target_include_directories(aemu-gl-qt-bridge PRIVATE .)
