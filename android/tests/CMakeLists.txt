# And the end to end test
set(hello-emulator_src end2end/HelloEmulator.cpp)

set(hello-emulator_src
  end2end/HelloEmulator.cpp)
android_add_test(hello-emulator)
target_link_libraries(hello-emulator PRIVATE android-emu gmock_main android-hw-config)
add_dependencies(hello-emulator emulator)

if (WINDOWS_X86_64 OR WINDOWS_MSVC_X86_64)
  android_nasm_compile(TARGET hello_world_asm_lib SOURCES yasm/hello_world.asm)
  set(hello_world_asm_test_src yasm/hello_world_unittest.cpp)
  android_add_test(hello_world_asm_test)
  target_link_libraries(hello_world_asm_test PRIVATE hello_world_asm_lib gmock_main)
endif()

add_subdirectory(end2end/test_crash_symbols)
if (NOT WINDOWS_X86_64)
  # The compiler tests BREAK the mingw compiler, so we just exclude them.
  # ../android/tests/compiler/longjump_test.c: In function ‘long_jump_preserve_int_params’:
  #../android/tests/compiler/longjump_test.c:20:1: internal compiler error: in mark_jump_label_1, at jump.c:1132
  add_subdirectory(compiler)
endif()
