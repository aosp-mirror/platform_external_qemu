# And the end to end test
set(hello-emulator_src end2end/HelloEmulator.cpp)
android_add_test(hello-emulator)
target_link_libraries(hello-emulator PRIVATE android-emu gmock_main)
add_dependencies(hello-emulator emulator)

add_subdirectory(end2end/test_crash_symbols)