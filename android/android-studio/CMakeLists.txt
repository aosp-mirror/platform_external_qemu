# We need a javac..
find_package(Java REQUIRED)
include(UseJava)

set(CMAKE_JNI_TARGET TRUE)
set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.8" "-target" "1.8")

add_jar(
  emulator-studio-view
  # cmake-format: sortable
  android/emulation/studio/EmulatorLayoutManager.java
  android/emulation/studio/EmulatorView.java
  android/emulation/studio/InteractiveEmulatorView.java
  android/emulation/studio/KeyMapper.java
  android/emulation/studio/NativeLibLoader.java
  android/emulation/studio/SampleWindow.java
  android/emulation/studio/SharedMemory.java
  android/emulation/studio/net/EmulatorListener.java
  android/emulation/studio/net/EmulatorTelnetClient.java
  ENTRY_POINT android/emulation/studio/SampleWindow)

create_javah(
  TARGET emulator-studio-view-headers
  CLASSES android.emulation.studio.SharedMemory CLASSPATH emulator-studio-view
  OUTPUT_NAME ${CMAKE_CURRENT_BINARY_DIR}/studio-view-headers.h)

set(android-studio_src android/emulation/studio/SharedMemory.cpp
                       ${CMAKE_CURRENT_BINARY_DIR}/studio-view-headers.h)
android_add_library(
  TARGET android-studio SHARED LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                      ${android-studio_src})
target_link_libraries(android-studio PRIVATE android-emu)

target_include_directories(android-studio PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared(android-studio)
install_jar(emulator-studio-view lib64)
