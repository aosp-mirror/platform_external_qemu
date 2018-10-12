cmake_minimum_required(VERSION 3.5)
# We can remove the if statement below once we no longer need the frankenstein
# building of make & cmake
if(NOT ANDROID_TARGET_TAG)
  get_filename_component(ADD_PATH
                         "${CMAKE_CURRENT_LIST_DIR}/android/build/cmake"
                         ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(frankenbuild)
endif()
message("Erasing flags: c: ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
include(android)
include(prebuilts)

# In our frankenbuild this is passed in, we can remove this later.
if(NOT ANDROID_FRANKENBUILD)
  if(EMULATOR_CRASHUPLOAD)
    set(ANDROID_EMULATOR_CRASHUPLOAD ${EMULATOR_CRASHUPLOAD})
  else()
    set(ANDROID_EMULATOR_CRASHUPLOAD "NONE")
  endif()
  # Make sure we don't use exceptions
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# We want to make sure we binplace everything in the root, so that gnumake can
# pick up all our created artifacts
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Let's not go nuts with reporting.
set(CMAKE_RULE_MESSAGES OFF)
set(CMAKE_TARGET_MESSAGES OFF)

prebuilt(CURL)
prebuilt(GLIB2)
prebuilt(USB)
prebuilt(ZLIB)
prebuilt(PIXMAN)
prebuilt(VIRGLRENDERER)
prebuilt(TCMALLOC)

# This defines all the include and def dependencies needed to compile the
# libqemu libraries
set(
  android-qemu-deps_includes_interface
  tcg
  tcg/i386
  accel/tcg
  include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLIB2_INCLUDE_DIRS}
  ${USB_INCLUDE_DIRS}
  ${PIXMAN_INCLUDE_DIRS}
  ${VIRGLRENDERER_INCLUDE_DIRS}
  qemu2-auto-generated
  ../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}/include
  android-qemu2-glue/config/${ANDROID_TARGET_TAG}/)

set(android-qemu-deps_defs_interface # Enable faster migration code when saving
                                     # RAM to a snapshot
    "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION"
    # Enable VIRGL
    "-DCONFIG_VIRGL"
    "-D__packed=__attribute((packed))"
    "-D_GNU_SOURCE"
    ${CURL_DEFINITIONS}
    "-D_FILE_OFFSET_BITS=64")
set(android-qemu-deps_compile_options_interface
    "-Werror=implicit-function-declaration"
    "-Wno-unused-function"
    "-fno-strict-aliasing"
    "-fno-common")
set(android-qemu-deps_libs_interface
    ${GLIB2_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${ZLIB_LIBRARIES})

# Darwin definitions
set(ANDROID_QEMU_i386_STUBS_darwin-x86_64 "")
set(ANDROID_QEMU_arm_STUBS_darwin-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(android-qemu-deps_darwin-x86_64_compile_options_interface
    "-Wno-address-of-packed-member"
    "-Wno-format-security"
    "-Wno-initializer-overrides"
    "-Wno-tautological-compare"
    "-Wno-tautological-pointer-compare"
    "-Wno-unused-variable")
set(android-qemu-deps_darwin-x86_64_defs_interface "-D_DARWIN_C_SOURCE=1")
set(android-qemu-deps_darwin-x86_64_libs_interface "-framework IOKit"
    "-framework CoreFoundation")

# Windows x86 definitions
set(ANDROID_QEMU_arm_STUBS_windows-x86 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_i386_STUBS_windows-x86 "hvf-stub.c")
set(android-qemu-deps_windows-x86_compile_options_interface
    "-Werror=implicit-function-declaration"
    "-mms-bitfields"
    "-Wno-unused-variable"
    "-Wno-unused-but-set-variable"
    "-Wno-maybe-uninitialized"
    "-m32")
set(android-qemu-deps_windows-x86_interface
    "-DWIN32_LEAN_AND_MEAN"
    "-DWINVER=0x601"
    "-D_WIN32_WINNT=0x601"
    "-D__USE_MINGW_ANSI_STDIO=1")

# Windows x86_64 definitions
set(ANDROID_QEMU_arm_STUBS_windows-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_i386_STUBS_windows-x86_64 "hvf-stub.c")
set(android-qemu-deps_windows-x86_64_compile_options_interface
    "-Werror=implicit-function-declaration"
    "-mms-bitfields"
    "-Wno-unused-function"
    "-Wno-unused-variable"
    "-Wno-unused-but-set-variable"
    "-Wno-maybe-uninitialized")
set(android-qemu-deps_windows-x86_64_interface
    "-DSTDC_LIMIT_MACROS=1"
    "-DWIN32_LEAN_AND_MEAN"
    "-DWINVER=0x601"
    "-D_LARGEFILE_SOURCE"
    "-D_WIN32_WINNT=0x601"
    "-D__USE_MINGW_ANSI_STDIO=1")

# Linux settings
set(ANDROID_QEMU_i386_STUBS_linux-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_arm_STUBS_linux-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(android-qemu-deps_linux-x86_64_includes_interface linux-headers)
set(android-qemu-deps_linux-x86_64_compile_options_interface
    "-Wno-address-of-packed-member"
    "-Wno-format-security"
    "-Wno-initializer-overrides"
    "-Wno-maybe-uninitialized"
    "-Wno-tautological-compare"
    "-Wno-tautological-pointer-compare"
    "-Wno-unused-but-set-variable"
    "-Wno-unused-variable")
set(android-qemu-deps_linux-x86_64_libs_interface ${TCMALLOC_LIBRARIES})

# Now we can declare the dependency library that sets all the properties for the
# qemu targets
add_android_interface_library(android-qemu-deps)

set(ANDROID_AUTOGEN "qemu2-auto-generated")
set(ANDROID_QEMU2_TOP_DIR ${ANDROID_QEMU2_TOP_DIR})
include(cmake-main.${ANDROID_TARGET_TAG}.inc)

set(libqemu2-common_src ${qemu2-shared_sources}
    ${qemu2-shared_generated_sources})
set(libqemu2-common-defs_private "-DPOISON_CONFIG_ANDROID")
set(libqemu2-common_libs_public android-qemu-deps)
add_android_library(libqemu2-common)

set(libqemu2-util_src ${libqemuutil_sources} ${libqemuutil_generated_sources})
set(libqemu2-util_libs_public android-qemu-deps)
set(libqemu2-util-defs_private "-DPOISON_CONFIG_ANDROID")
add_android_library(libqemu2-util)

set(libqemustub_src ${libqemustub_sources} ${libqemuutil_generated_sources})
set(libqemustub_libs_public android-qemu-deps)
add_android_library(libqemustub)

# Qemu-img
set(qemu-img_src ${qemu-img_sources})
set(qemu-img_libs_private libqemu2-util libqemustub android-qemu-deps)
add_android_executable(qemu-img)

# Qemu - i386
set(libqemu2-system-i386_src
    ${qemu-system-i386_sources}
    ${qemu-system-i386_generated_sources}
    ${ANDROID_QEMU_i386_STUBS_${ANDROID_TARGET_TAG}})
set(libqemu2-system-i386_includes_private android-qemu2-glue/config/target-x86
    target/i386)
set(libqemu2-system-i386_defs_private "-DNEED_CPU_H" "-DCONFIG_ANDROID")
set(libqemu2-system-i386_libs_private android-qemu-deps)
add_android_library(libqemu2-system-i386)

# Qemu - x86_64
set(libqemu2-system-x86_64_src
    ${qemu-system-x86_64_sources}
    ${qemu-system-x86_64_generated_sources}
    ${ANDROID_QEMU_i386_STUBS_${ANDROID_TARGET_TAG}})
set(libqemu2-system-x86_64_includes_private
    android-qemu2-glue/config/target-x86_64 target/i386)
set(libqemu2-system-x86_64_defs_private "-DNEED_CPU_H" "-DCONFIG_ANDROID")
set(libqemu2-system-x86_64_libs_private android-qemu-deps)
add_android_library(libqemu2-system-x86_64)

# Qemu - armel
set(libqemu2-system-armel_src
    ${qemu-system-arm_sources}
    ${qemu-system-arm_generated_sources}
    ${ANDROID_QEMU_arm_STUBS_${ANDROID_TARGET_TAG}})
set(libqemu2-system-armel_includes_private
    disas/libvixl
    android-qemu2-glue/config/target-arm
    target/arm)
set(libqemu2-system-armel_defs_private "-DNEED_CPU_H" "-DCONFIG_ANDROID")
set(libqemu2-system-armel_libs_private android-qemu-deps)
add_android_library(libqemu2-system-armel)
# Qemu - aarch64
set(libqemu2-system-aarch64_src
    ${qemu-system-aarch64_sources}
    ${qemu-system-aarch64_generated_sources}
    ${ANDROID_QEMU_arm_STUBS_${ANDROID_TARGET_TAG}})
set(libqemu2-system-aarch64_includes_private
    disas/libvixl
    android-qemu2-glue/config/target-arm64
    target/arm)
set(libqemu2-system-aarch64_defs_private "-DNEED_CPU_H" "-DCONFIG_ANDROID")
set(libqemu2-system-aarch64_libs_private android-qemu-deps)
add_android_library(libqemu2-system-aarch64)

set(libqemu-test_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
set(libqemu-test_libs_public android-qemu-deps)
add_android_library(libqemu-test)

set(libqemu-test-crypto_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
set(libqemu-test-crypto_libs_public android-qemu-deps)
add_android_library(libqemu-test-crypto)

# Adds a qemu test, note that these test should never
# bring in an android dependency
function(add_qtest NAME DEPENDENCY)
  set(${NAME}_qtest_src tests/${NAME}.c)
  set(${NAME}_qtest_includes_public ${ANDROID_AUTOGEN}/tests)
  set(${NAME}_qtest_libs_private
      ${DEPENDENCY}
      libqemu2-util
      libqemu2-common
      android-qemu-deps)
  add_android_test(${NAME}_qtest)
endfunction()

# These tests rely on interaction with the qemu binary. These require a non-
# trivial test runner as they interact with a running emulator. It is not yet
# clear how we can run these.
set(TEST_NEED_QEMU
    test-arm-mptimer
    test-crypto-block
    test-bdrv-drain
    test-block-backend
    test-blockjob
    test-blockjob-txn
    test-char
    test-filter-mirror
    test-filter-redirector
    test-hbitmap
    test-hmp
    test-io-channel-buffer
    test-io-channel-command
    test-io-channel-file
    test-io-channel-socket
    test-io-task
    test-netfilter
    test-qdev-global-props
    test-qga
    test-replication
    test-thread-pool
    test-throttle
    test-vmstate
    test-write-threshold
    test-x86-cpuid-compat
    test-xbzrle
    test-crypto-tlscredsx509
    test-crypto-tlssession
    test-crypto-secret)

# These require different set of linkers.
set(CRYPTO_TESTS
    test-crypto-afsplit
    test-crypto-cipher
    test-crypto-hash
    test-crypto-hmac
    test-crypto-ivgen
    test-crypto-pbkdf
    test-crypto-xts)

# These tests are currently failing.
set(SIMPLE_TEST_FAILURES test-qht-par)

set(SIMPLE_TESTS
    test-aio
    test-base64
    test-bitcnt
    test-bitops
    test-bufferiszero
    test-clone-visitor
    test-coroutine
    test-cutils
    test-int128
    test-iov
    test-keyval
    test-logging
    test-mul64
    test-opts-visitor
    test-qapi-util
    test-qdist
    test-qemu-opts
    test-qht
    test-qmp-event
    test-qobject-input-visitor
    test-qobject-output-visitor
    test-rcu-list
    test-shift128
    test-string-input-visitor
    test-string-output-visitor
    test-timed-average
    test-uuid
    test-visitor-serialization
    test-x86-cpuid)

# Add basic tests
foreach(test ${SIMPLE_TESTS})
  add_qtest(${test} libqemu-test)
endforeach()

# Add crypto tests
foreach(test ${CRYPTO_TESTS})
  add_qtest(${test} libqemu-test-crypto)
endforeach()


# Socket test is split in two, so we special case it
set(test-util-sockets-qtest_src tests/test-util-sockets.c
    tests/socket-helpers.c)
set(test-util-sockets-qtest_includes_public ${ANDROID_AUTOGEN}/tests)
set(test-util-sockets-qtest_libs_private
    libqemu2-util
    libqemu2-common
    android-qemu-deps)
add_android_test(test-util-sockets-qtest)

