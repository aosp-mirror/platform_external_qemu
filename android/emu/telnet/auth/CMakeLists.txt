android_add_library(
  TARGET android-emu-telnet-console-auth
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/console_auth.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-proxy android-emu-utils)
target_include_directories(android-emu-telnet-console-auth PRIVATE src
                           PUBLIC include)

android_add_test(
  TARGET android-emu-telnet-console-auth_unittests
  SOURCE_DIR test
  SRC android/console_auth_unittest.cpp
  DEPS android-emu-base android-emu-base-headers gmock gmock_main)
target_include_directories(android-emu-telnet-console-auth_unittests PRIVATE src test)
target_link_libraries(android-emu-telnet-console-auth_unittests PRIVATE android-emu-telnet-console-auth)

