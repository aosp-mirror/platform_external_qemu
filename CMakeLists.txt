cmake_minimum_required(VERSION 3.5)
# We can remove the if statement below once we no longer need the frankenstein
# building of make & cmake
if(NOT ANDROID_TARGET_TAG)
  get_filename_component(ADD_PATH
                         "${CMAKE_CURRENT_LIST_DIR}/android/build/cmake"
                         ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(frankenbuild)
  # Forcefully erase cflags to fix compiler issues/warnings.
  #  message(FATAL_ERROR "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(CMAKE_CXX_FLAGS "")
endif()

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
prebuilt(ZLIB)
prebuilt(PIXMAN)
prebuilt(VIRGLRENDERER)
prebuilt(TCMALLOC)
prebuilt(OpenSSL)

# There is no USB support on windows, so we will not try to resolve it.
if (NOT ${ANDROID_TARGET_TAG} MATCHES "windows.*")
    prebuilt(USB)
endif()

# This defines all the include directories needed to compile the
# libqemu libraries. There should be no need to make additions here.
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

# These are the rules that every qemu component needs, regardless of the
# platform you are compiling on. Please document any flags you set as not
# everyone is a compiler expert!
set(android-qemu-deps_defs_interface "-D_GNU_SOURCE"
    # Enable faster migration code when saving RAM to a snapshot
    "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION"
    # Enable VIRGL
    "-DCONFIG_VIRGL"
    "-D__packed=__attribute((packed))"
    # We link against the static version of curl
    ${CURL_DEFINITIONS}
    "-D_FILE_OFFSET_BITS=64")
set(android-qemu-deps_compile_options_interface
    "-Werror=implicit-function-declaration" "-Wno-unused-function"
    # Do not allow the compiler to assume the strictest aliasing rules
    # applicable to the language being compiled. For C (and C++), this activates
    # optimizations based on the type of expressions. In particular, an object
    # of one type is assumed never to reside at the same address as an object of
    # a different type, unless the types are almost the same.  Supposedly this
    # can introduce weird failures esp. on older gcc versions (MINGW)
    # (See http://thiemonagel.de/2010/01/no-strict-aliasing/)
    "-fno-strict-aliasing"
    # This option instructs the compiler to assume that signed arithmetic
    # overflow of addition, subtraction and multiplication wraps around using
    # twos-complement representation. This flag enables some optimizations and
    # disables others. The options -ftrapv and -fwrapv override each other, so
    # using -ftrapv -fwrapv on the command-line results in -fwrapv being
    # effective. Qemu is using this, so we use it as well.
    "-fwrapv"
    # The -fno-common option specifies that the compiler should instead place
    # uninitialized global variables in the data section of the object file.
    # This inhibits the merging of tentative definitions by the linker so you
    # get a multiple-definition error if the same variable is defined in more
    # than one compilation unit. Compiling with -fno-common is useful on targets
    # for which it provides better performance, or if you wish to verify that
    # the program will work on other systems that always treat uninitialized
    # variable definitions this way.
    # Remove this and you'll get all sorts of fun linker issues.
    "-fno-common")


# Make sure we make our dependent libraries available.
set(android-qemu-deps_libs_interface
    ${GLIB2_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${ZLIB_LIBRARIES})

# Darwin definitions
set(ANDROID_QEMU_i386_STUBS_darwin-x86_64 "")
set(ANDROID_QEMU_arm_STUBS_darwin-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(android-qemu-deps_darwin-x86_64_compile_options_interface
    # Emit extra code to check for buffer overflows, such as stack smashing
    # attacks. This is done by adding a guard variable to functions with
    # vulnerable objects. This includes functions that call alloca, and
    # functions with buffers larger than 8 bytes. The guards are initialized
    # when a function is entered and then checked when the function exits. If a
    # guard check fails, an error message is printed and the program exits.
    # Like -fstack-protector but includes additional functions to be protected —
    # those that have local array definitions, or have references to local frame
    # addresses.
    "-fstack-protector-strong"
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
set(android-qemu-deps_windows-x86_defs_interface
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
set(android-qemu-deps_windows-x86_64_defs_interface
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
    # Emit extra code to check for buffer overflows, such as stack smashing
    # attacks. This is done by adding a guard variable to functions with
    # vulnerable objects. This includes functions that call alloca, and
    # functions with buffers larger than 8 bytes. The guards are initialized
    # when a function is entered and then checked when the function exits. If a
    # guard check fails, an error message is printed and the program exits.
    # Like -fstack-protector but includes additional functions to be protected —
    # those that have local array definitions, or have references to local frame
    # addresses.
    "-fstack-protector-strong"
    "-Wno-address-of-packed-member"
    "-Wno-format-security"
    "-Wno-initializer-overrides"
    "-Wno-tautological-compare"
    "-Wno-tautological-pointer-compare"
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
set(libqemu2-util_windows-x86_libs_public -lpsapi)
set(libqemu2-util_windows-x86_64_libs_public -lpsapi)
set(libqemu2-util-defs_private "-DPOISON_CONFIG_ANDROID")
add_android_library(libqemu2-util)

set(libqemustub_src ${libqemustub_sources} ${libqemuutil_generated_sources})
set(libqemustub_libs_public android-qemu-deps)
add_android_library(libqemustub)

# Qemu-img
set(qemu-img_src ${qemu-img_sources}
   block/qcow2-bitmap.c)
set(qemu-img_linux-x86_64_src
     scsi/pr-manager.c)
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

# The tests will not compile on windows yet..
if (NOT ${ANDROID_TARGET_TAG} MATCHES "windows.*")
    include(qemu-test.cmake)
endif()

