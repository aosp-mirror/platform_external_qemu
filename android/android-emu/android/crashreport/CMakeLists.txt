cmake_minimum_required(VERSION 2.8.11)
project(CRASHREPORT)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)
list(APPEND CMAKE_MODULE_PATH "${REPO_ROOT}/external/qemu/android/cmake")

include(AddCXXCompilerFlag)
include(AddGlobalVariables)

add_global_variables()

add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-std=c++1y)
add_cxx_compiler_flag(-fPIC)
add_cxx_compiler_flag(-w)


#TODO(jansene): Would be nice if we could specify this.
add_definitions(-DCRASHUPLOAD=NONE)


list(APPEND SOURCES
  # testing/main-test-crasher.cpp
  ui/ConfirmDialog.ui
  ui/ConfirmDialog.cpp
  CrashReporter_common.cpp
  CrashService_common.cpp
  # CrashService_unittest.cpp
  CrashSystem.cpp
  CrashSystem.h
  # CrashSystem_unittest.cpp
  main-crash-service.cpp)


if(APPLE)
  list(APPEND SOURCES CrashReporter_darwin.cpp)
  FIND_LIBRARY(CARBON_LIBRARY Carbon)
  FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
  MARK_AS_ADVANCED (CARBON_LIBRARY APP_SERVICES_LIBRARY)
  SET(EXTRA_LIBS ${CARBON_LIBRARY} ${APP_SERVICES_LIBRARY})
elseif(UNIX)
  list(APPEND SOURCES CrashReporter_linux.cpp)
elseif(WIN32)
  list(APPEND SOURCES CrashReporter_windows.cpp CrashService_windows.cpp)
endif()

add_subdirectory(${ANDROID_EMU}/android/metrics/proto proto)
add_subdirectory(${ANDROID_EMU}/android/base base)

include_directories(
  ${ANDROID_EMU}
  ${CURL_INCLUDE_DIRS}
  ${BREAKPAD_INCLUDE_DIRS}
  ${QT5_INCLUDE_DIRS}
  ${METRICS_PROTO_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  )

message("Include: ${QT5_INCLUDE}")

set(BREAKPAD_STATIC_LIBS
  ${ANDROID_EMULATOR_BUILD}/common/breakpad/${BUILD_HOST_TAG64}/lib/libbreakpad.a
  ${ANDROID_EMULATOR_BUILD}/common/breakpad/${BUILD_HOST_TAG64}/lib/libbreakpad_client.a
  ${ANDROID_EMULATOR_BUILD}/common/breakpad/${BUILD_HOST_TAG64}/lib/libdisasm.a
  )
set(LIB_CURL_STATIC_LIBS
  ${ANDROID_EMULATOR_BUILD}/curl/${BUILD_HOST_TAG64}/lib/libcurl.a
  ${ANDROID_EMULATOR_BUILD}/curl/${BUILD_HOST_TAG64}/lib/libssl.a
  ${ANDROID_EMULATOR_BUILD}/curl/${BUILD_HOST_TAG64}/lib/libcrypto.a
  ${ANDROID_EMULATOR_BUILD}/common/lz4/${BUILD_HOST_TAG64}/lib/liblz4.a
  ${ANDROID_EMULATOR_BUILD}/qemu-android-deps/${BUILD_HOST_TAG64}/lib/libz.a
  )



add_executable(emulator-crash-service ${SOURCES} ${GENERATED_SOURCES})
target_link_libraries(emulator-crash-service ${BREAKPAD_STATIC_LIBS}
  ${EXTRA_LIBS} base metrics_proto  ${LIB_CURL_STATIC_LIBS})
add_qt_support(emulator-crash-service)

add_dependencies(emulator-crash-service metrics_proto)
add_dependencies(emulator-crash-service base)
