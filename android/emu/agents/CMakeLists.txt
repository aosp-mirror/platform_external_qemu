add_library(android-emu-agents-headers INTERFACE)
target_include_directories(android-emu-agents-headers INTERFACE include)
if(OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(android-emu-agents-headers
                        INTERFACE aemu-host-common.headers)
endif()
android_license(TARGET "android-emu-agents-headers" LIBNAME None SPDX None
                LICENSE Apache-2.0 LOCAL None)

android_add_library(
  TARGET android-emu-agents
  SHARED
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/AndroidAgentFactory.cpp
      android/emulation/control/LineConsumer.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-files
       android-hw-config)

android_target_properties(android-emu-agents darwin
                          "INSTALL_RPATH>=@loader_path/gles_swiftshader")
target_include_directories(
  android-emu-agents
  PUBLIC include
  PRIVATE
    src
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})
target_link_libraries(android-emu-agents PUBLIC android-emu-agents-headers
                                                android-emu-utils)

target_compile_options(android-emu-agents PRIVATE "-Wno-extern-c-compat")
target_compile_definitions(android-emu-agents PRIVATE "CONSOLE_EXPORTS")
android_install_shared(android-emu-agents)

android_add_test(
  TARGET android-emu-agents_unittests
  SOURCE_DIR test
  SRC android/emulation/control/LineConsumer_unittest.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers gmock
       gmock_main gtest)
target_include_directories(android-emu-agents_unittests PRIVATE src)
