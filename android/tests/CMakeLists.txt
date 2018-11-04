
# And the end to end test

android_generate_hw_config()

set(hello-emulator_src 
  ${ANDROID_HW_CONFIG_H}
  end2end/HelloEmulator.cpp)
android_add_test(hello-emulator)
target_link_libraries(hello-emulator PRIVATE android-emu gmock_main)
add_dependencies(hello-emulator emulator)