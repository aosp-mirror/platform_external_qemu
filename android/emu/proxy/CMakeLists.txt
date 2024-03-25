# A library that contains proxy related things
android_add_library(
  TARGET android-emu-proxy
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/http_proxy.c
      android/proxy/proxy_common.c
      android/proxy/proxy_http.c
      android/proxy/proxy_http_connector.c
      android/proxy/proxy_http_rewriter.c
      android/proxy/proxy_setup.cpp
      android/proxy/ProxyUtils.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers android-emu-utils)
target_include_directories(android-emu-proxy PRIVATE src PUBLIC include)

if(ANDROID_EMULATOR_BUILD)
  android_add_test(
    TARGET android-emu-proxy_unittests
    SOURCE_DIR test
    SRC android/proxy/proxy_common_unittest.cpp
        android/proxy/ProxyUtils_unittest.cpp
    DEPS android-emu-base android-emu-base-headers gmock gmock_main)
  target_include_directories(android-emu-proxy_unittests PRIVATE src test)
  target_link_libraries(android-emu-proxy_unittests PRIVATE android-emu-proxy)
endif()
