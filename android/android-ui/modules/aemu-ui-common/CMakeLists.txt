android_qt_lib(
  TARGET aemu-ui-common
  # SHARED
  ALIAS ui::common
  SRC android/framebuffer.c
      android/skin/charmap.c
      android/skin/file.c
      android/skin/generic-event-buffer.cpp
      android/skin/generic-event.cpp
      android/skin/image.c
      android/skin/keyboard.c
      android/skin/keycode-buffer.c
      android/skin/keycode.c
      android/skin/qt/function-runner.cpp
      android/skin/resource.c
      android/skin/trackball.c
      android/test/checkboot.cpp
  DEPS android-emu-agents android-emu-base android-emu-files android-emu-utils
       qemu-host-common-headers)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  target_sources(aemu-ui-common
                 PRIVATE src/android/skin/qt/mac-native-window.mm)
  target_link_libraries(aemu-ui-common PRIVATE "-framework Carbon")
endif()

# TODO(jansene): We should create X11 targets, as aarch64 needs the X11 libs from QT5 path
# keyboard.c needs X11
android_target_link_libraries(aemu-gl-init linux-aarch64 PRIVATE -lX11 -lXau ${QT5_LINK_PATH})
android_target_link_libraries(aemu-gl-init linux-x86_64 PRIVATE -lX11)
