android_add_library(
  TARGET jsoncpp
  LICENSE MIT
  URL  "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/android/android-webrtc/third_party/jsoncpp/LICENSE"
  SOURCE_DIR source/src/lib_json/
  SRC # cmake-format: sortable
      json_reader.cpp json_value.cpp
      json_writer.cpp)
target_compile_definitions(jsoncpp PUBLIC JSON_USE_EXCEPTION=0
                                           JSON_USE_NULLREF=0)
target_include_directories(jsoncpp PUBLIC source/include source/include/json
# Some folks expect us in third_party/jsoncpp/json.h
../..)
target_compile_options(jsoncpp PRIVATE -Wno-implicit-fallthrough)

android_add_executable(TARGET jsoncpp_test
SOURCE_DIR source/src/test_lib_json/
NODISTRIBUTE
SRC
    jsontest.cpp
    jsontest.h
    fuzz.cpp
    fuzz.h
    main.cpp
)
target_link_libraries(jsoncpp_test PRIVATE jsoncpp)
add_test(
    NAME jsoncpp_test
    COMMAND
      $<TARGET_FILE:jsoncpp_test> --test-auto)
