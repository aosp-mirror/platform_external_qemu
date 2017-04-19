cmake_minimum_required(VERSION 2.8.11)
project(ANDROID)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckCXXCompilerFlag)
include(AddCXXCompilerFlag)
include(FindProtobuf)

add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-Wextra)
add_cxx_compiler_flag(-Wshadow)
add_cxx_compiler_flag(-Werror RELEASE)
add_cxx_compiler_flag(-Werror RELWITHDEBINFO)
add_cxx_compiler_flag(-Werror MINSIZEREL)
add_cxx_compiler_flag(-pedantic)
add_cxx_compiler_flag(-Wshorten-64-to-32)
add_cxx_compiler_flag(-Wfloat-equal)
add_cxx_compiler_flag(-fstrict-aliasing)
add_cxx_compiler_flag(-std=c++1y)
add_cxx_compiler_flag(-fPIC)
add_cxx_compiler_flag(-w)

if(APPLE)
  set(BUILD_HOST_TAG64 darwin-x86_64)
  add_definitions(-D_DARWIN_C_SOURCE)
elseif(UNIX)
  set(BUILD_HOST_TAG64 linux-x86_64)
elseif(WIN32)
  set(BUILD_HOST_TAG64 windows-x86_64)
  # set(BUILD_HOST_TAG64 windows-x86) TODO FIX
else()
  message(FATAL_ERROR "Unsupported OS")
endif()


set(ANDROID_ROOT ${ANDROID_SOURCE_DIR}/../../..)
set(PREBUILTS ${ANDROID_ROOT}/prebuilts)
set(EXTERNAL ${ANDROID_ROOT}/external)
set(QEMU ${EXTERNAL}/qemu)
set(ANDROID_EMULATOR_BUILD ${PREBUILTS}/android-emulator-build)
set(QEMU_DEPS ${ANDROID_EMULATOR_BUILD}/qemu-android-deps/${BUILD_HOST_TAG64})
set(Protobuf_PROTOC_EXECUTABLE ${ANDROID_EMULATOR_BUILD}/protobuf/${BUILD_HOST_TAG64}/bin/protoc)
set(CONFIG_HOST_INCLUDE_DIRS ${QEMU}/android-qemu2-glue/config/${BUILD_HOST_TAG64})
# set(CMAKE_AUTOMOC ON)


add_definitions(-D__STDC_LIMIT_MACROS=1)
link_directories(${QEMU_DEPS}/lib)

include_directories(
	# TODO(jansene): Clean this up
  ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/breakpad/${BUILD_HOST_TAG64}/include/breakpad
  ${ANDROID_EMULATOR_BUILD}/common/e2fsprogs/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/ffmpeg/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/libvpx/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/libxml2/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/lz4/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/swiftshader/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/common/x264/${BUILD_HOST_TAG64}/include
  ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtCore
  ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtWidgets
  ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtGui
  ${ANDROID_EMULATOR_BUILD}/qt/common/include
  ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/include.system
  ${ANDROID_EMULATOR_BUILD}/curl/${BUILD_HOST_TAG64}/include

  ${ANDROID_EMULATOR_BUILD}/protobuf/${BUILD_HOST_TAG64}/include
  ${EXTERNAL}/google-benchmark/include
  ${QEMU_DEPS}/include
  ${PREBUILTS}/external/google-benchmark/include
  android-emu
  android-emugl/host/include
  )

add_subdirectory(third_party)
add_subdirectory(android-emugl)
add_subdirectory(android-emu)

# add_executable(emulator emulator/main-emulator.cpp)
