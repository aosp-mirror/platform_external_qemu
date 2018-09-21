cmake_minimum_required(VERSION 3.5)
project("Android-Emulator")

if(NOT DEFINED ANDROID_QEMU2_TOP_DIR)
  # So, you are not using the android toolchain aye
  message(WARNING "Not using the android toolchain!")
  if(APPLE)
    set(ANDROID_TARGET_TAG "darwin-x86_64")
    set(ANDROID_TARGET_OS "darwin")
  elseif(UNIX)
    set(ANDROID_TARGET_TAG "linux-x86_64")
    set(ANDROID_TARGET_OS "linux")
  elseif(WIN32)
    set(ANDROID_TARGET_TAG "windows-x86_64")
    set(ANDROID_TARGET_OS "windows")
  endif()
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
    include
    .
    qemu2-auto-generated
    ${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}/include
    ${PIXMAN_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${VIRGLRENDERER_INCLUDE_DIRS}
    ${USB_INCLUDE_DIRS})

target_compile_options(android-qemu-deps
                           INTERFACE
                            -fno-strict-aliasing
                            -fno-common)

target_compile_definitions(android-qemu-deps
    INTERFACE
    -D__STDC_CONSTANT_MACROS
    -D_LIBCPP_VERSION=__GLIBCPP__
    -D__STDC_LIMIT_MACROS=1
    -D__STDC_FORMAT_MACROS=1
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE_SOURCE
    -D_GNU_SOURCE=1
    "-D__packed=__attribute\(\(packed\)\)"
    -DANDROID_SDK_TOOLS_CL_SHA1=${ANDROID_QEMU_SHA}
    -DENABLE_DLOG=0
    -DCRASHUPLOAD=NONE
    -DCURL_STATICLIB
    -D_GNU_SOURCE
    -D_FILE_OFFSET_BITS=64
    -DCONFIG_MIGRATION_RAM_SINGLE_ITERATION
    -DCONFIG_VIRGL
)

target_link_libraries(android-qemu-deps
                      INTERFACE
                                android-emu-base
                                emulator-libui
                                ${LIBZ_LIBRARIES}
                                ${USB_LIBRARIES}
                                ${VIRGLRENDERER_LIBRARIES}
                                emulator-tinyepoxy
                                -lfdt
                                )

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
  # Nothing..
  target_compile_definitions(android-qemu-deps
                           INTERFACE
                           -D_DARWIN_C_SOURCE=1
                           -Dftello64=ftell
                           -Dfseeko64=fseek
                           )
  target_include_directories(android-qemu-deps
                             INTERFACE disas/libvixl)
  target_link_libraries(android-qemu-deps
                      INTERFACE
                     "-Wl,-weak_framework,Hypervisor"
                     "-framework CoreAudio"
                     "-framework VideoToolbox"
                     "-framework AVFoundation"
                     "-framework VideoDecodeAcceleration"
                      -lbz2 -liconv)
  set(ANDROID_QEMU_i386_STUBS "dummy.c")
  set(ANDROID_QEMU_arm_STUBS "hvf-stub.c")
  set(ANDROID_QEMU_LAST_LINK "-lglib-2.0;-lutil")
elseif(WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")
  target_include_directories(android-qemu-deps INTERFACE .)
elseif(UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
  target_include_directories(android-qemu-deps
                             INTERFACE linux-headers linux-headers/asm86)
 target_link_libraries(android-qemu-deps
                         INTERFACE emulator-libui
                                -lX11)
  set(ANDROID_QEMU_i386_STUBS hvf-stub.c)
  set(ANDROID_QEMU_arm_STUBS hvf-stub.c)
  set(ANDROID_QEMU_LAST_LINK "-lglib-2.0;-lpulse;-lutil")
endif()

get_git_version(ANDROID_QEMU_VERSION)
get_git_sha(ANDROID_QEMU_SHA)


include(cmake-main.${ANDROID_TARGET_TAG}.inc)

configure_file(qemu-version.h.in qemu-version.h @ONLY)

add_library(qemuutil ${libqemuutil_sources} ${libqemuutil_generated_sources})
target_link_libraries(qemuutil PRIVATE android-qemu-deps)

add_library(qemu2-shared ${qemu2-shared_sources} ${qemu2-shared_generated_sources})
target_link_libraries(qemu2-shared PRIVATE android-qemu-deps)

create_qemu_target(i386 x86 ${ANDROID_QEMU_i386_STUBS} i386 ${ANDROID_QEMU_LAST_LINK})
create_qemu_target(x86_64 x86_64  ${ANDROID_QEMU_i386_STUBS} i386 ${ANDROID_QEMU_LAST_LINK})
create_qemu_target(aarch64 arm64  ${ANDROID_QEMU_arm_STUBS} arm ${ANDROID_QEMU_LAST_LINK})
create_qemu_target(arm arm  ${ANDROID_QEMU_arm_STUBS}  arm ${ANDROID_QEMU_LAST_LINK})
