set(JSONCPP_ROOT ${AOSP_ROOT}/external/jsoncpp)
android_add_library(
  TARGET jsoncpp
  LICENSE
    MIT
    URL
    "https://android.googlesource.com/platform/external/jsoncpp/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SOURCE_DIR ${JSONCPP_ROOT}/src/lib_json/
  SRC # cmake-format: sortable
      json_reader.cpp json_value.cpp json_writer.cpp)
# Some folks expect json.h us in third_party/jsoncpp/json.h
android_symlink(${JSONCPP_ROOT}/include/json jsoncpp
                ${CMAKE_CURRENT_BINARY_DIR}/third_party)
android_symlink("${AOSP_ROOT}/external/googletest/googletest/" "gtest" ${CMAKE_CURRENT_BINARY_DIR}/testing)
android_symlink("${AOSP_ROOT}/external/googletest/googlemock/" "gmock" ${CMAKE_CURRENT_BINARY_DIR}/testing)
target_compile_definitions(jsoncpp PUBLIC JSON_USE_EXCEPTION=0
                                          JSON_USE_NULLREF=0)
target_include_directories(
  jsoncpp PUBLIC ${JSONCPP_ROOT}/include ${JSONCPP_ROOT}/include/json
                 ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(jsoncpp PRIVATE -Wno-implicit-fallthrough)

# Disabled due to use of exceptions in unit tests.
if(FALSE)
  android_add_executable(
    TARGET jsoncpp_test SOURCE_DIR ${JSONCPP_ROOT}/src/test_lib_json/
    NODISTRIBUTE SRC fuzz.cpp fuzz.h jsontest.cpp jsontest.h main.cpp)
  target_link_libraries(jsoncpp_test PRIVATE jsoncpp)
  target_compile_options(jsoncpp_test PRIVATE -fexceptions)
  add_test(NAME jsoncpp_test COMMAND $<TARGET_FILE:jsoncpp_test> --test-auto)
endif()
