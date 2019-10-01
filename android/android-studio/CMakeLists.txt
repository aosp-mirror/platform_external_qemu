# We need a javac..
find_package(Java REQUIRED)
include(UseJava)

set(CMAKE_JNI_TARGET TRUE)
set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.8" "-target" "1.8")

add_jar(emulator-studio-view
        android/emulation/studio/EmulatorView.java
        android/emulation/studio/InteractiveEmulatorView.java
        android/emulation/studio/KeyMapper.java
        android/emulation/studio/NativeLibLoader.java
        android/emulation/studio/SampleWindow.java
        android/emulation/studio/SharedMemory.java
        android/emulation/studio/net/EmulatorListener.java
        android/emulation/studio/net/EmulatorTelnetClient.java
        ENTRY_POINT
        android/emulation/studio/SampleWindow
        GENERATE_NATIVE_HEADERS   # Note that generate headers is only available in cmake >= 3.11
        studio-native)

set(android-studio_src android/emulation/studio/SharedMemory.cpp)


if(${CMAKE_VERSION} VERSION_LESS "3.11.0")
  # Newer versions of java no longer support this command.
  # If you are seeing errors you will have to use a newer version of CMake
  # in combination with a newer version of Java.

  if (Java_VERSION_MAJOR VERSION_GREATER "9")
        message(FATAL_ERROR "Javah has been removed in JDK 10, you will need to upgrade your cmake version to 3.11 or higher.")
  endif()
  create_javah(TARGET
               emulator-studio-view-headers
               CLASSES
               android.emulation.studio.SharedMemory
               CLASSPATH
               emulator-studio-view
               OUTPUT_NAME
               ${CMAKE_CURRENT_BINARY_DIR}/android_emulation_studio_SharedMemory.h)
  list(APPEND android-studio_src ${CMAKE_CURRENT_BINARY_DIR}/android_emulation_studio_SharedMemory.h)
else()
  set(ANDROID_STUDIO_DEPS studio-native)
endif()

android_add_shared_library(android-studio)
target_link_libraries(android-studio PRIVATE android-emu ${ANDROID_STUDIO_DEPS})

target_include_directories(android-studio PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared(android-studio)
install_jar(emulator-studio-view lib64)
