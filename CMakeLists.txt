cmake_minimum_required(VERSION 3.5)
project("Android-Emulator")

# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge to the android build system.
string(REPLACE " "
               ";"
               INCLUDES
               ${LOCAL_C_INCLUDES})

# Use same compiler config as android build system
set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
set(CMAKE_CXX_FLAGS "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

# Work around some gcc/mingw issues
if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Make sure we create archives in the same way..
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

add_library(android-qemu-deps INTERFACE)

target_include_directories(
  android-qemu-deps
  INTERFACE
    tcg
    tcg/i386
    accel/tcg
    include
    .
    qemu2-auto-generated
    ${INCLUDES}
    ../../prebuilts/android-emulator-build/qemu-android-deps/${LOCAL_TARGET_TAG}/include
    android-qemu2-glue/config/${LOCAL_TARGET_TAG}/)

target_compile_definitions(android-qemu-deps
                           INTERFACE
                           -DNEED_CPU_H
                           -DPOISON_CONFIG_ANDROID)

if(${LOCAL_TARGET_TAG} STREQUAL "darwin-x86_64")
  set(ANDROID_QEMU_i386_STUBS "")
  set(ANDROID_QEMU_arm_STUBS "hvf-stub.c;accel/stubs/hax-stub.c")
  target_compile_options(android-qemu-deps
                         INTERFACE "-fno-strict-aliasing" "-fno-common"
                                   "-Wno-address-of-packed-member")

  target_compile_definitions(android-qemu-deps INTERFACE "-D_DARWIN_C_SOURCE=1")
elseif(${LOCAL_TARGET_TAG} STREQUAL "windows-x86")
  set(ANDROID_QEMU_arm_STUBS "hvf-stub.c;accel/stubs/hax-stub.c")
  set(ANDROID_QEMU_i386_STUBS "hvf-stub.c")
  target_compile_options(android-qemu-deps
                         INTERFACE "-Werror=implicit-function-declaration"
                                   "-mms-bitfields"
                                   "-fno-strict-aliasing"
                                   "-fno-common"
                                   "-Wno-unused-function"
                                   "-Wno-unused-variable"
                                   "-Wno-unused-but-set-variable"
                                   "-Wno-maybe-uninitialized"
                                   "-m32")
  target_compile_definitions(android-qemu-deps
                             INTERFACE
                             "-DCURL_STATICLIB"
                             "-DENABLE_DLOG=0"
                             "-DWIN32_LEAN_AND_MEAN"
                             "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION"
                             "-DCONFIG_VIRGL"
                             "-DWINVER=0x601"
                             "-D_FILE_OFFSET_BITS=64"
                             "-D_GNU_SOURCE"
                             "-D_GNU_SOURCE=1"
                             "-D_LARGEFILE_SOURCE"
                             "-D_WIN32_WINNT=0x601"
                             "-D__USE_MINGW_ANSI_STDIO=1"
                             "-D__packed=__attribute((packed))")
elseif(${LOCAL_TARGET_TAG} STREQUAL "windows-x86_64")
  set(ANDROID_QEMU_arm_STUBS "hvf-stub.c;accel/stubs/hax-stub.c")
  set(ANDROID_QEMU_i386_STUBS "hvf-stub.c")
  target_compile_options(android-qemu-deps
                         INTERFACE "-Werror=implicit-function-declaration"
                                   "-mms-bitfields"
                                   "-fno-strict-aliasing"
                                   "-fno-common"
                                   "-Wno-unused-function"
                                   "-Wno-unused-variable"
                                   "-Wno-unused-but-set-variable"
                                   "-Wno-maybe-uninitialized")
  target_compile_definitions(android-qemu-deps
                             INTERFACE
                             "-DCURL_STATICLIB"
                             "-DENABLE_DLOG=0"
                             "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION"
                             "-DCONFIG_VIRGL"
                             "-DSTDC_LIMIT_MACROS=1"
                             "-DWIN32_LEAN_AND_MEAN"
                             "-DWINVER=0x601"
                             "-D_FILE_OFFSET_BITS=64"
                             "-D_GNU_SOURCE"
                             "-D_GNU_SOURCE=1"
                             "-D_LARGEFILE_SOURCE"
                             "-D_WIN32_WINNT=0x601"
                             "-D__STDC_FORMAT_MACROS=1"
                             "-D__USE_MINGW_ANSI_STDIO=1"
                             "-D__packed=__attribute\(\(packed\)\)")
elseif(${LOCAL_TARGET_TAG} STREQUAL "linux-x86_64")
  set(ANDROID_QEMU_i386_STUBS "hvf-stub.c;accel/stubs/hax-stub.c")
  set(ANDROID_QEMU_arm_STUBS "hvf-stub.c;accel/stubs/hax-stub.c")
  target_include_directories(android-qemu-deps INTERFACE linux-headers)
  target_compile_options(android-qemu-deps
                         INTERFACE -Wno-address-of-packed-member)
endif()

set(ANDROID_AUTOGEN "qemu2-auto-generated")
set(ANDROID_QEMU2_TOP_DIR ${LOCAL_QEMU2_TOP_DIR})
include(cmake-main.${LOCAL_TARGET_TAG}.inc)

add_library(libqemu2-common ${qemu2-shared_sources}
            ${qemu2-shared_generated_sources})
target_include_directories(
  libqemu2-common
  PRIVATE android-qemu2-glue/config/${LOCAL_TARGET_TAG}/
          ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86_64
          ${INCLUDES}
          include)

add_library(libqemu2-util ${libqemuutil_sources}
            ${libqemuutil_generated_sources})
target_include_directories(
  libqemu2-util
  PRIVATE android-qemu2-glue/config/${LOCAL_TARGET_TAG}/
          ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86_64
          ${INCLUDES}
          include)

add_library(libqemu2-system-i386
            ${qemu-system-i386_sources}
            ${qemu-system-i386_generated_sources}
            ${ANDROID_QEMU_i386_STUBS})
add_library(libqemu2-system-x86_64
            ${qemu-system-x86_64_sources}
            ${qemu-system-x86_64_generated_sources}
            ${ANDROID_QEMU_i386_STUBS})
add_library(libqemu2-system-armel
            ${qemu-system-arm_sources}
            ${qemu-system-arm_generated_sources}
            ${ANDROID_QEMU_arm_STUBS})
add_library(libqemu2-system-aarch64
            ${qemu-system-aarch64_sources}
            ${qemu-system-aarch64_generated_sources}
            ${ANDROID_QEMU_arm_STUBS})

target_include_directories(
  libqemu2-system-i386
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86
          ${ANDROID_QEMU2_TOP_DIR}/target/i386)

target_link_libraries(libqemu2-system-i386 PRIVATE android-qemu-deps)

target_include_directories(
  libqemu2-system-x86_64
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86_64
          ${ANDROID_QEMU2_TOP_DIR}/target/i386)
target_link_libraries(libqemu2-system-x86_64 PRIVATE android-qemu-deps)

target_include_directories(
  libqemu2-system-armel
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/disas/libvixl
          ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-arm
          ${ANDROID_QEMU2_TOP_DIR}/target/arm)
target_link_libraries(libqemu2-system-armel PRIVATE android-qemu-deps)

target_include_directories(
  libqemu2-system-aarch64
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/disas/libvixl
          ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-arm64
          ${ANDROID_QEMU2_TOP_DIR}/target/arm)
target_link_libraries(libqemu2-system-aarch64 PRIVATE android-qemu-deps)
