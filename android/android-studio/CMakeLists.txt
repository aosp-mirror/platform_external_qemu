# We need jni..
find_package(JNI REQUIRED)
find_package(Java REQUIRED)
include(UseJava)

set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.8" "-target" "1.8")

add_jar(emulator-studio-view
        android/emulation/studio/SharedMemory.java
        android/emulation/studio/EmulatorView.java
        android/emulation/studio/SampleWindow.java
        ENTRY_POINT
        android/emulation/studio/SampleWindow)


create_javah(TARGET
             emulator-studio-view-headers
             CLASSES
             android.emulation.studio.SharedMemory
             CLASSPATH
             emulator-studio-view
             OUTPUT_NAME
             ${CMAKE_CURRENT_BINARY_DIR}/studio-view-headers.h)

set(android-studio_src android/emulation/studio/SharedMemory.cpp ${CMAKE_CURRENT_BINARY_DIR}/studio-view-headers.h)
android_add_shared_library(android-studio)
target_link_libraries(android-studio PRIVATE android-emu)

target_include_directories(android-studio PRIVATE ${JNI_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared(android-studio)
install_jar(emulator-studio-view lib64)

# set(android-studio_unittest_src) android_add_test(android-studio_unittest) target_link_libraries(android-
# studio_unittest PRIVATE android-studio gmock_main)
