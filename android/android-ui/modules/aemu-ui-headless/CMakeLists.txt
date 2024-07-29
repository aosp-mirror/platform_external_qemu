# Version of libui without Qt
android_qt_lib(
  TARGET aemu-ui-headless
  ALIAS ui::headless
  SRC android/skin/event-headless.cpp
      android/skin/LibuiAgent.cpp
      android/skin/qt/emulator-no-qt-no-window.cpp
      android/skin/surface-headless.cpp
      android/skin/winsys-headless.cpp
      android/window-agent-headless-impl.cpp
  DEPS android-emu ui::common ui::window)

target_compile_definitions(aemu-ui-headless PRIVATE CONFIG_HEADLESS)

if(NOT BUILDING_FOR_AARCH64)
  target_compile_definitions(aemu-ui-headless PRIVATE USE_MMX=1)
  target_compile_options(aemu-ui-headless PRIVATE -mmmx)
endif()
