cmake_minimum_required(VERSION 3.5)
project("Android-Emulator")

if(NOT DEFINED ANDROID_QEMU2_TOP_DIR)
  # So, you are not using the android toolchain aye
  message(WARNING "Not using the android toolchain!")
  set(ANDROID_TARGET_TAG "linux-x86_64")
  set(ANDROID_TARGET_OS "linux")
  get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}"
                         DIRECTORY)
  get_filename_component(ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake"
                         ABSOLUTE)
  message(STATUS "Using ${ANDROID_QEMU2_TOP_DIR} - MODULE: ${ADD_PATH}")
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

endif()

include(git)
include(qemu-builder)
include(prebuilts)

set(OPTION_CRASHUPLOAD "NONE" CACHE STRING "Destination of crash reports.")
set(OPTION_SDK_TOOLS_REVISION ""
    CACHE STRING "The tools revision to use, if any")
set(OPTION_SDK_TOOLS_BUILD_NUMBER ""
    CACHE STRING "The sdk buildnumber to use, if any.")

set_property(CACHE OPTION_CRASHUPLOAD
             PROPERTY STRINGS
                      NONE
                      STAGING
                      PROD)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fno-exceptions -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -DGOOGLE_PROTOBUF_NO_RTTI")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -DANDROID_DEBUG")
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(
      CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping -fprofile-arcs -ftest-coverage --coverage"
      )
  endif()
else()
  if(WIN32 OR ANDROID_TARGET_TAG MATCHES "windows.*")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG=1")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG=1")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if(APPLE OR ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")

elseif(WIN32 OR ANDROID_TARGET_TAG MATCHES "windows.*")
  set(
    CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -DWIN32_LEAN_AND_MEAN -D__USE_MINGW_ANSI_STDIO=1 -mms-bitfields"
    )
elseif(UNIX OR ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
include(prebuilts)

add_subdirectory(android/third_party)
add_subdirectory(android-qemu2-glue)
add_subdirectory(android/android-emu)
add_subdirectory(android/android-emugl)
add_subdirectory(android/emulator)
add_subdirectory(android/emulator-check)

set(ANDROID_AUTOGEN qemu2-auto-generated/)

# Now configure QEMU dependencies
prebuilt(GLIB2)
prebuilt(PIXMAN)
prebuilt(VIRGLRENDERER)
prebuilt(USB)
add_library(android-qemu-deps INTERFACE)

target_include_directories(
  android-qemu-deps
  INTERFACE
    android-qemu2-glue/config/${ANDROID_TARGET_TAG}
    tcg
    tcg/i386
    accel/tcg
    linux-headers
    linux-headers/asm86
    include
    /usr/local/google/home/jansene/src/emu-master-dev/prebuilts/android-emulator-build/qemu-android-deps/linux-x86_64/include
    .
    qemu2-auto-generated
    ${PIXMAN_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${VIRGLRENDERER_INCLUDE_DIRS}
    ${USB_INCLUDE_DIRS})

get_git_version(ANDROID_QEMU_VERSION)
get_git_sha(ANDROID_QEMU_SHA)

target_compile_definitions(android-qemu-deps
                           INTERFACE
                           -DANDROID_SDK_TOOLS_CL_SHA1=${ANDROID_QEMU_SHA}
                           -DCONFIG_MIGRATION_RAM_SINGLE_ITERATION
                           -DCONFIG_VIRGL
                           -DCRASHUPLOAD=${OPTION_CRASHUPLOAD}
                           -DCURL_STATICLIB
                           -DENABLE_DLOG=0
                           -DNDEBUG=1
                           -DSNAPSHOT_PROFILE=0
                           -D_FILE_OFFSET_BITS=64
                           -D_GNU_SOURCE=1
                           -D_LARGEFILE_SOURCE
                           -D_LIBCPP_VERSION=__GLIBCPP__
                           -D__STDC_CONSTANT_MACROS
                           -D__STDC_FORMAT_MACROS=1
                           -D__STDC_LIMIT_MACROS=1
                           "-D__packed=__attribute\(\ (packed\) \)")

target_link_libraries(android-qemu-deps
                      INTERFACE emulator-libui
                                ${LIBZ_LIBRARIES}
                                ${USB_LIBRARIES}
                                ${VIRGLRENDERER_LIBRARIES}
                                emulator-tinyepoxy
                                -lX11
                                -lfdt)

include(cmake-main.${ANDROID_TARGET_TAG}.inc)

configure_file(qemu-version.h.in qemu-version.h @ONLY)
add_library(qemuutil ${libqemuutil_sources} ${libqemuutil_generated_sources})
target_link_libraries(qemuutil PRIVATE android-qemu-deps)

create_qemu_target(i386 x86 "hvf-stub.c" i386 "-lglib-2.0;-lpulse;-lutil")
create_qemu_target(x86_64 x86_64 "hvf-stub.c" i386 "-lglib-2.0;-lpulse;-lutil")
create_qemu_target(aarch64 arm64 "hvf-stub.c" arm "-lglib-2.0;-lpulse;-lutil")
create_qemu_target(arm arm "hvf-stub.c" arm "-lglib-2.0;-lpulse;-lutil")
