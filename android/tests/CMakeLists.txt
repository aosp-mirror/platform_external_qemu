# And the end to end test
android_add_test(TARGET hello-emulator SRC # cmake-format: sortable
                                           end2end/HelloEmulator.cpp)
target_link_libraries(hello-emulator PRIVATE android-emu android-emu-base-headers gmock_main
                                             android-hw-config)
add_dependencies(hello-emulator emulator)
add_subdirectory(end2end/test_crash_symbols)
