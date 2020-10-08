project(Android-Emulator)
cmake_minimum_required(VERSION 3.5)
message("Android target tag: ${ANDROID_TARGET_TAG}")

if(NOT DEFINED ANDROID_TARGET_TAG)
  message(
    WARNING
      "You need to invoke the cmake generator with a proper toolchain from android/build/cmake, "
      "It is best to invoke: android/scripts/rebuild. "
      "Using the system toolchain might have unintended consequences, and you will need all required "
      "packages to build qemu (https://wiki.qemu.org/Hosts/Linux,https://wiki.qemu.org/Hosts/Mac)."
  )
  list(APPEND CMAKE_MODULE_PATH
       "${CMAKE_CURRENT_LIST_DIR}/android/build/cmake/")
  include(toolchain)
  _get_host_tag(TAG)
  toolchain_configure_tags(${TAG})
endif()

message(
  STATUS
    "Configured host ${ANDROID_HOST_TAG} to target: ${ANDROID_TARGET_TAG}, target os: ${ANDROID_TARGET_OS}/${ANDROID_TARGET_OS_FLAVOR}"
)

include(android)
include(prebuilts)

set(OPTION_CRASHUPLOAD "NONE" CACHE STRING "Destination of crash reports.")
set(OPTION_SDK_TOOLS_REVISION "" CACHE STRING
                                       "The tools revision to use, if any")
set(OPTION_SDK_TOOLS_BUILD_NUMBER ""
    CACHE STRING "The sdk buildnumber to use, if any.")
set(OPTION_ASAN "" CACHE STRING "The list of build sanitizers to user")
set(OPTION_COVERAGE_IN_DEBUG TRUE
    CACHE BOOL "True if code coverage should be enabled in debug builds.")
set(OPTION_ASAN_IN_DEBUG TRUE
    CACHE BOOL "True if asan should be turned on for debug builds.")
set(OPTION_TIME_INFO FALSE
    CACHE BOOL "True if you wish to collect compile time statistics")
set(OPTION_CLANG_TIDY
    ""
    CACHE
      STRING
      "List of targets for which you wish to run clang-tidy for code analysis.")
set(OPTION_CLANG_TIDY_FIX FALSE
    CACHE BOOL "True if you wish clang-tidy to auto fix issues it encounters.")
set(QTWEBENGINE FALSE CACHE BOOL "True if the webengine should be enabled.")
if(LINUX_X86_64)
  set(WEBRTC
      TRUE
      CACHE
        BOOL
        "True if the emulator should be build with WEBRTC support. Defaults on for Linux"
  )
else()
  set(WEBRTC
      FALSE
      CACHE
        BOOL
        "True if the emulator should be build with WEBRTC support. Defaults on for Linux"
  )
endif()
set(QEMU_UPSTREAM TRUE
    CACHE BOOL "True if the upstream version of qemu should be build.")
set(ANDROIDSTUDIO FALSE
    CACHE BOOL
          "Deprecated. Android studio is using the gRPC interface directly.")
set(OPTION_MINBUILD
    FALSE
    CACHE
      BOOL
      "True if we should build a minimum set of components to support 64-bit guets only."
)
set(OPTION_WIN_LTO TRUE
    CACHE BOOL "True if we should enable global link optimizations in windows (slow)")
set(OPTION_TCMALLOC
    TRUE
    CACHE
      BOOL
      "True if we should use TCMalloc on its supported platforms (currently, linux)."
)
set(OPTION_GFXSTREAM_BACKEND
    FALSE
    CACHE
      BOOL
      "True if we should build the gfxstream backend (plus everything else).")

set(GFXSTREAM
    FALSE
    CACHE
      BOOL
      "True if we should build gfxstream, crosvm, and its accompanying libraries/tests."
)

set(OPTION_AEMU_LIBS_ONLY FALSE
    CACHE BOOL "True if we should build only AEMU libraries and their tests.")

set(CROSVM FALSE CACHE BOOL
                       "True if we should build crosvm (also builds gfxstream)")

set(GFXSTREAM_ONLY FALSE CACHE BOOL "True if we should build only gfxstream")

if(WINDOWS_MSVC_X86_64)
  set(OPTION_PERFETTO_USE_SDK FALSE
      CACHE BOOL "True if we should be building host side tracing on top of the official Perfetto SDK. Defaults false for Windows as it doesn't build on Windows.")
else()
  # Currently FALSE due to needing more integration work for the full SDK.
  set(OPTION_PERFETTO_USE_SDK FALSE
      CACHE BOOL "True if we should be building host side tracing on top of the official Perfetto SDK. Defaults false for Windows as it doesn't build on Windows.")
endif()


if(GFXSTREAM_ONLY)
  set(CROSVM FALSE)
  set(GFXSTREAM TRUE)
endif()

if(LINUX_AARCH64)
  set(OPTION_MINBUILD TRUE)
  set(QTWEBENGINE FALSE)
  set(QEMU_UPSTREAM FALSE)
  set(WEBRTC FALSE)
endif()

if(CROSVM OR GFXSTREAM)
  set(OPTION_AEMU_LIBS_ONLY TRUE)
  set(OPTION_GFXSTREAM_BACKEND TRUE)
  set(OPTION_TCMALLOC FALSE)
  set(WEBRTC FALSE)
  set(ANDROIDSTUDIO FALSE)
endif()

string(STRIP "${OPTION_SDK_TOOLS_BUILD_NUMBER}" OPTION_SDK_TOOLS_BUILD_NUMBER)
string(STRIP "${OPTION_SDK_TOOLS_REVISION}" OPTION_SDK_TOOLS_REVISION)

set_property(CACHE OPTION_CRASHUPLOAD PROPERTY STRINGS NONE STAGING PROD)
set(DBG_INFO ${CMAKE_BINARY_DIR}/build/debug_info)

# Symbol extraction is not supported when cross compiling.
if(OPTION_CRASHUPLOAD STREQUAL "NONE")
  set(ANDROID_EXTRACT_SYMBOLS FALSE)
else()
  set(ANDROID_EXTRACT_SYMBOLS TRUE)
  set(ANDROID_SYMBOL_DIR ${DBG_INFO})
  file(MAKE_DIRECTORY ${ANDROID_SYMBOL_DIR})
  message(STATUS "Uploading symbols to ${OPTION_CRASHUPLOAD}")
endif()

if(ANDROID_EXTRACT_SYMBOLS AND CROSSCOMPILE)
  message(
    WARNING
      "Symbol extraction not supported when cross compiling from ${ANDROID_HOST_TAG} -> ${ANDROID_TARGET_TAG}, disabling."
  )
  set(ANDROID_EXTRACT_SYMBOLS FALSE)
endif()

# Make sure targets have complete control of include order.
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

# Enable C++17 in C++ (Note, cmake does not set the proper flags when using
# CXX_STANDARD) with our custom clang.
add_cxx_flag("-std=c++17")

# Gather compile time statistics
if(OPTION_TIME_INFO)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Let's bin place everything in the root, with the shared libs in the right
# place
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archives)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${DBG_INFO})
# Feeling courageous? Set this to $ANDROID_SDK_ROOT
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/distribution/emulator)

# Pthreads from the prebuilts please!
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
prebuilt(Threads)

# Configure OS depdendency licensenses
if(LINUX_AARCH64)
  message("Cross build aarch64 libstdc++")
  android_license(
    TARGET "RUNTIME_OS_DEPENDENCIES"
    LIBNAME "libstdc++"
    URL "https://packages.debian.org/sid/crossbuild-essential-arm64"
    SPDX "LLVM-exception"
    LICENSE
      "https://android.googlesource.com/toolchain/clang/+/refs/heads/master/LICENSE.TXT"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LLVM")
elseif(LINUX_X86_64)
  android_license(
    TARGET "RUNTIME_OS_DEPENDENCIES"
    LIBNAME "libc++"
    URL "https://android.googlesource.com/toolchain/clang/+/refs/heads/master/"
    SPDX "LLVM-exception"
    LICENSE
      "https://android.googlesource.com/toolchain/clang/+/refs/heads/master/LICENSE.TXT"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LLVM")
else()
  android_license(TARGET "RUNTIME_OS_DEPENDENCIES" LIBNAME None SPDX None
                  LICENSE None LOCAL None)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions("-DANDROID_DEBUG")
  add_c_flag("-O0 -g3")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CROSSCOMPILE)
    if(NOT OPTION_ASAN AND OPTION_ASAN_IN_DEBUG)
      set(OPTION_ASAN address)
    endif()

    if(OPTION_ASAN STREQUAL "thread" AND OPTION_COVERAGE_IN_DEBUG)
      message(FATAL_ERROR "You cannot run tsan with code coverage enabled.")
    endif()
    if(NOT WINDOWS_MSVC_X86_64 AND OPTION_COVERAGE_IN_DEBUG)
      message("Enabling code coverage")
      # Build an instrumented version of the code  that generates coverage
      # mapping to enable code coverage analysis
      set(ANDROID_CODE_COVERAGE TRUE)
      add_c_flag("-fcoverage-mapping")
      add_c_flag("-fprofile-instr-generate")
      add_c_flag("-fprofile-arcs")
      add_c_flag("-ftest-coverage")
      add_c_flag("--coverage")
    endif()
  endif()
else()
  set(CMAKE_INSTALL_DO_STRIP TRUE)
  add_definitions("-DNDEBUG=1")
  if(WINDOWS_MSVC_X86_64)
    # clang-cl takes msvc based parameters, so -O3 is a nop
    add_c_flag("-O2")
  else()
    add_c_flag("-O3 -g3")
  endif()
endif()

# Target specific configurations that we do not want to do in the
# toolchain.cmake Toolchain variables seem to be overwritten pending your cmake
# version.
if(LINUX_X86_64)
  add_c_flag("-Werror")
  add_c_flag("-Wno-deprecated-declarations") # Protobuf generates deprecation
                                             # warnings for deprecated enums
  # And the asm type if we are compiling with yasm
  set(ANDROID_NASM_TYPE elf64)
  # This should make sure we have sufficient information left to properly print
  # std::string etc. see b/156534499 for details.
  add_c_flag("-fno-limit-debug-info")
elseif(LINUX_AARCH64)
  set(ANDROID_NASM_TYPE elf64)
elseif(WINDOWS_MSVC_X86_64)
  # And the asm type if we are compiling with yasm
  set(ANDROID_NASM_TYPE win64)
  set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
elseif(DARWIN_X86_64)
  # And the asm type if we are compiling with yasm
  set(ANDROID_NASM_TYPE macho64)
  # Always consider the source to be darwin.
  add_definitions(-D_DARWIN_C_SOURCE=1)
  add_c_flag("-Wno-everything")
else()
  message(FATAL_ERROR "Unknown target!")
endif()

# Note that clang in windows act as a drop in replacement for cl
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions("-D__STDC_CONSTANT_MACROS" "-D_LIBCPP_VERSION=__GLIBCPP__")

  # If macro expansion goes wrong, we like to see why
  add_c_flag("-fmacro-backtrace-limit=0")

  # Let's not trip over warnings & errors that do not exist on every platform
  # clang-cl != clang-lin != clang-darwin
  add_c_flag("-Wno-unknown-warning-option")
  add_c_flag("-Wno-unknown-argument")
endif()

if(DARWIN_X86_64)
  add_c_flag("-Werror")
endif()

if(WINDOWS_MSVC_X86_64)
  add_c_flag("-MD")
  # b/141774858 to track turning them back on again.
  add_c_flag("-Wno-everything")
  add_definitions("-D_NO_EXCEPTIONS")

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Disable security checks, "we know what we are doing"
    add_c_flag("-GS-")
    # Disable rtti, we are not going to be needing it, this is the msvc
    # equivalent of -fno-rtti
    add_c_flag("-GR-")

    # Link time optization tragically fails when cross compiling.
    if(NOT CROSSCOMPILE AND OPTION_WIN_LTO)
      # Do a global optimization pass (note, link times --> infinity)
      add_c_flag("-flto=thin")
      # Optimize a bit more on the vtables in use, note this requires -flto
      add_c_flag("-fwhole-program-vtables")
    endif()
  endif()
endif()

# Copy the source properties and add the build number to it, this will enable
# external tools to easily operate on version id.
set(SOURCE_PROP_DEST ${CMAKE_BINARY_DIR}/source.properties)
file(COPY ${ANDROID_QEMU2_TOP_DIR}/source.properties
     DESTINATION ${CMAKE_BINARY_DIR})
file(APPEND ${SOURCE_PROP_DEST} "Pkg.BuildId=${OPTION_SDK_TOOLS_BUILD_NUMBER}")
install(FILES ${SOURCE_PROP_DEST} DESTINATION . RENAME source.properties)

if(OPTION_CLANG_TIDY)
  set(DO_FIX "")
  if(OPTION_CLANG_TIDY_FIX)
    set(DO_FIX "--fix")
  endif()
  # Workaround to ignore generated code.
  file(WRITE "${PROJECT_BINARY_DIR}/.clang-tidy"
       "Checks: '-*,android-cloexec-accept'")
  set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "${DO_FIX}")
endif()

add_c_flag("-g")
# Disable generation of information about every class with virtual functions for
# use by the C++ runtime type identification features (dynamic_cast and typeid).
# If you don't use those parts of the language, you can save some space by using
# this flag. Note that exception handling uses the same information, but it will
# generate it as needed. The  dynamic_cast operator can still be used for casts
# that do not require runtime type information, i.e. casts to void * or to
# unambiguous base classes.
if(NOT WINDOWS_MSVC_X86_64)
  add_cxx_flag("-fno-rtti")
endif()

# Compiling with -fno-exceptions disables exceptions support and uses the
# variant of C++ libraries without exceptions. Use of try, catch, or throw
# results in an error message. Linking objects that have been compiled with
# -fno- exceptions automatically selects the libraries without exceptions. You
# can use the linker option --no_exceptions to diagnose whether the objects
# being linked contain exceptions. If an exception propagates into a function
# that has been compiled without exceptions support, then the program
# terminates.
add_cxx_flag("-fno-exceptions")
add_cxx_flag("-Wno-invalid-offsetof")
add_cxx_flag("-Wno-implicit-int-float-conversion")

# Ensure that <inttypes.h> always defines all interesting macros.
add_definitions("-D__STDC_LIMIT_MACROS=1" "-D__STDC_FORMAT_MACROS=1")
# (From https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-
# imply) with GNU_SOURCE, you will get:
#
# * access to lots of nonstandard GNU/Linux extension functions
# * access to traditional functions which were omitted from the POSIX standard
#   (often for good reason, such as being replaced with better alternatives, or
#   being tied to particular legacy implementations)
# * access to low-level functions that cannot be portable, but that you
#   sometimes need for implementing system utilities like mount, ifconfig, etc.
#   broken behavior for lots of POSIX-specified functions, where the GNU folks
#   disagreed with the standards committee on how the functions should behave
#   and decided to do their own thing. As long as you're aware of these things,
#   it should not be a problem to define _GNU_SOURCE, but you should avoid
#   defining it and instead define _POSIX_C_SOURCE=200809L or _XOPEN_SOURCE=700
#   when possible to ensure that your programs are portable.
add_definitions("-D_GNU_SOURCE=1")

# Enable large-file support (i.e. make off_t a 64-bit value).
add_definitions("-D_FILE_OFFSET_BITS=64" "-D_LARGEFILE_SOURCE")

if(OPTION_ASAN AND (LINUX_X86_64 OR DARWIN_X86_64))
  message(STATUS "Enabling ASAN with: ${OPTION_ASAN}")
  add_c_flag("-fsanitize=${OPTION_ASAN}")
  add_c_flag("-g3")
  if(OPTION_ASAN STREQUAL "address")
    add_c_flag("-fno-omit-frame-pointer")
  endif()
endif()

# Toolchain file can get loaded multiple times, resulting in this variable being
# overwritten on native windows.
if(MSVC AND WINDOWS_MSVC_X86_64)
  set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
endif()

# We now have configured our base compiler..

# Unit tests please!
enable_testing()

# TODO(jansene): Make these prebuilts. Files on which the emulator (and some
# unit tests dependes). If a target takes a prebuilt_dependency on these, then
# they will be binplaced in  ${EXECUTABLE_DIRECTORY}/lib/...
set(EMULATOR_FEATURE_DEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/android/data/advancedFeatures.ini>lib/advancedFeatures.ini"
    "${ANDROID_QEMU2_TOP_DIR}/android/data/advancedFeaturesCanary.ini>lib/advancedFeaturesCanary.ini"
    "${ANDROID_QEMU2_TOP_DIR}/android/data/emu-original-feature-flags.protobuf>lib/emu-original-feature-flags.protobuf"
    "${ANDROID_QEMU2_TOP_DIR}/android/data/hostapd.conf>lib/hostapd.conf")

if(LINUX_AARCH64)
  set(ANDROID_TARGET_ARCH "aarch64")
else()
  set(ANDROID_TARGET_ARCH "x86_64")
endif()

set(SWIFTSHADER_DEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}/lib/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}/lib/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}/lib/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

android_license(
  TARGET SWIFTSHADER_DEPENDENCIES
  LIBNAME swiftshader
  EXECUTABLES
    "gles_swiftshader/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "gles_swiftshader/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "gles_swiftshader/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
  URL "https://android.googlesource.com/platform/external/swiftshader/+/refs/heads/emu-master-dev"
  SPDX "Apache-2.0"
  LICENSE
    "https://android.googlesource.com/platform/external/swiftshader/+/refs/heads/emu-master-dev/LICENSE.txt"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.APACHE2")

if(NOT LINUX_AARCH64)
  # Angle shader translator (and possibly also vk renderer) dependencies
  set(ANGLE_DEPENDENCIES
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ANGLE/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}/lib/libshadertranslator${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/libshadertranslator${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

  android_license(
    TARGET ANGLE_DEPENDENCIES
    LIBNAME angle
    EXECUTABLES "libshadertranslator${CMAKE_SHARED_LIBRARY_SUFFIX}"
    URL "https://chromium.googlesource.com/angle/angle"
    SPDX "BSD-3-Clause"
    LICENSE "https://chromium.googlesource.com/angle/angle/LICENSE"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.ANGLE")

endif()

if(WINDOWS_MSVC_X86_64)
  set(ANGLE_RENDERER_DEPENDENCIES
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ANGLE/windows-x86_64/lib/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_angle/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ANGLE/windows-x86_64/lib/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_angle/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/ANGLE/windows-x86_64/lib/d3dcompiler_47${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_angle/d3dcompiler_47${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

  # Angle executables licensing information.
  android_license(
    TARGET ANGLE_RENDERER_DEPENDENCIES
    LIBNAME angle-renderer
    EXECUTABLES
      "gles_angle/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
      "gles_angle/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
      "gles_angle/d3dcompiler_47${CMAKE_SHARED_LIBRARY_SUFFIX}"
    URL "https://chromium.googlesource.com/angle/angle"
    SPDX "BSD-3-Clause"
    LICENSE "https://chromium.googlesource.com/angle/angle/LICENSE"
    LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.ANGLE")

endif()

prebuilt(GLIB2)
prebuilt(SDL2)
prebuilt(FDT)
prebuilt(PIXMAN)
prebuilt(VIRGLRENDERER)
prebuilt(TCMALLOC)

# Make sure the standard set of windows libraries are available as targets
if(WINDOWS)
  include(emu-windows-libs)
endif()

# Get the versions from git (Note that both functions are doing pretty much the
# same)
get_git_version(ANDROID_QEMU_VERSION)
get_git_sha(ANDROID_QEMU_SHA)

# Generate the qemu-version header.
configure_file(qemu-version.h.in qemu-version.h @ONLY)

# We need the auto generated header for some components, so let's set the
# ANDROID_HW_CONFIG_H variable to point to the generated header. Those that need
# it can add it to their sources list, and it will be there.
android_generate_hw_config()

# Lets set up a qemu dependency that we all can rely on
android_add_interface(
  TARGET android-qemu-deps
  LICENSE "GPL-2.0-only"
  LIBNAME
    qemu
    URL
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE")

# This defines all the include directories needed to compile the libqemu
# libraries. There should be no need to make additions here.
if(LINUX_AARCH64)
  set(TCG_ARCH_DIR "tcg/aarch64")
else()
  set(TCG_ARCH_DIR "tcg/i386")
endif()
target_include_directories(
  android-qemu-deps
  INTERFACE
    tcg
    ${TCG_ARCH_DIR}
    accel/tcg
    include
    disas/libvixl
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    qemu2-auto-generated
    ../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}/include
    android-qemu2-glue/config/${ANDROID_TARGET_TAG}/)

# These are the rules that every qemu component needs, regardless of the
# platform you are compiling on. Please document any flags you set as not
# everyone is a compiler expert!
target_compile_definitions(
  android-qemu-deps
  INTERFACE # Enable faster migration code when saving RAM to a snapshot
            "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION")
target_compile_definitions(android-qemu-deps INTERFACE # Enable VIRGL
                                                       "-DCONFIG_VIRGL")

target_compile_options(
  android-qemu-deps
  INTERFACE "-Werror=implicit-function-declaration"
            "-Wno-unused-function"
            "-Wno-address-of-packed-member"
            "-Wno-shift-negative-value"
            # Do not allow the compiler to assume the strictest aliasing rules
            # applicable to the language being compiled. For C (and C++), this
            # activates optimizations based on the type of expressions. In
            # particular, an object of one type is assumed never to reside at
            # the same address as an object of a different type, unless the
            # types are almost the same. Supposedly this can introduce weird
            # failures esp. on older gcc versions (MINGW) (See
            # http://thiemonagel.de/2010/01/no-strict-aliasing/)
            "-fno-strict-aliasing")

# This option instructs the compiler to assume that signed arithmetic overflow
# of addition, subtraction and multiplication wraps around using twos-complement
# representation. This flag enables some optimizations and disables others. The
# options -ftrapv and -fwrapv override each other, so using -ftrapv -fwrapv on
# the command-line results in -fwrapv being effective. Qemu is using this, so we
# use it as well. "-fwrapv" The -fno- common option specifies that the compiler
# should instead place uninitialized global variables in the data section of the
# object file.  This inhibits the merging of tentative definitions by the linker
# so you get a multiple- definition error if the same variable is defined in
# more than one compilation unit.  Compiling with -fno- common is useful on
# targets for which it provides better performance, or if you wish to verify
# that the program will work on other systems that always treat uninitialized
# variable definitions this way.  Remove this and you'll get all sorts of fun
# linker issues.
target_compile_options(android-qemu-deps INTERFACE "-fno-common")

# Make sure we make our dependent libraries available.
target_link_libraries(android-qemu-deps INTERFACE GLIB2::GLIB2 PIXMAN::PIXMAN
                                                  zlib)

target_link_libraries(android-qemu-deps INTERFACE virglrenderer-headers)

# Now we add the normal android libs, so we can use them
add_subdirectory(android)
add_subdirectory(android-qemu2-glue)

set(ANDROID_QEMU_ARM_DEVICES "hw/pci/goldfish_address_space.c")
set(ANDROID_QEMU_COMMON_DEVICES "${ANDROID_QEMU_ARM_DEVICES};hw/i386/pc_piix.c")

# Darwin definitions
set(ANDROID_QEMU_i386_DEVICES_darwin-x86_64
    "hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
set(ANDROID_QEMU_arm_DEVICES_darwin-x86_64
    "hvf-stub.c;accel/stubs/hax-stub.c;${ANDROID_QEMU_ARM_DEVICES}")
target_compile_definitions(android-qemu-deps INTERFACE "-DANDROID_IO")
android_target_compile_options(
  android-qemu-deps
  darwin-x86_64
  INTERFACE
  # Emit extra code to check for buffer overflows, such as stack smashing
  # attacks. This is done by adding a guard variable to functions with
  # vulnerable objects. This includes functions that call alloca, and functions
  # with buffers larger than 8 bytes. The guards are initialized when a function
  # is entered and then checked when the function exits. If a guard check fails,
  # an error message is printed and the program exits. Like -fstack-protector
  # but includes additional functions to be protected — those that have local
  # array definitions, or have references to local frame addresses.
  "-fstack-protector-strong"
  "-Wno-address-of-packed-member"
  "-Wno-format-security"
  "-Wno-initializer-overrides"
  "-Wno-tautological-compare"
  "-Wno-tautological-pointer-compare"
  "-Wno-unused-variable"
  "-Wno-return-type"
  "-Wno-shift-negative-value"
  # There are a series of out of bounds issues in hvf.
  "-Wno-array-bounds"
  "-Wno-format"
  "-Wno-implicit-int-float-conversion"
  "-Wno-reorder-init-list")
android_target_compile_definitions(android-qemu-deps darwin-x86_64 INTERFACE
                                   "-D_DARWIN_C_SOURCE=1")
android_target_link_libraries(android-qemu-deps darwin-x86_64 INTERFACE
                              "-framework IOKit" "-framework CoreFoundation")

# Windows x86 definitions
set(ANDROID_QEMU_arm_DEVICES_windows_msvc-x86_64
    "hvf-stub.c;accel/stubs/hax-stub.c;${ANDROID_QEMU_ARM_DEVICES}")
set(ANDROID_QEMU_i386_DEVICES_windows_msvc-x86_64
    "hvf-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
android_target_compile_options(
  android-qemu-deps
  windows_msvc-x86_64
  INTERFACE
  "-Wno-sometimes-uninitialized"
  "-Wno-return-type"
  "-Wno-initializer-overrides"
  "-Wno-int-to-void-pointer-cast"
  "-Wno-incompatible-pointer-types"
  "-Wno-unused-variable"
  "-Wno-microsoft-include"
  "-Wno-address-of-packed-member"
  "-Wno-address-of"
  "-mms-bitfields"
  "-mcx16"
  "-Wno-implicit-int-float-conversion"
  "-Wno-reorder-init-list")

android_target_compile_definitions(
  android-qemu-deps
  windows
  INTERFACE
  "-DWIN32_LEAN_AND_MEAN"
  "-DWINVER=0x601"
  "-D_WIN32_WINNT=0x601"
  "-D__USE_MINGW_ANSI_STDIO=1")

# Linux settings
set(ANDROID_QEMU_i386_DEVICES_linux-x86_64
    "hvf-stub.c;accel/stubs/hax-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}"
)
set(ANDROID_QEMU_arm_DEVICES_linux-x86_64
    "hvf-stub.c;accel/stubs/hax-stub.c;${ANDROID_QEMU_ARM_DEVICES}")

set(ANDROID_QEMU_i386_DEVICES_linux-aarch64
    "hvf-stub.c;accel/stubs/hax-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}"
)
set(ANDROID_QEMU_arm_DEVICES_linux-aarch64
    "hvf-stub.c;accel/stubs/hax-stub.c;${ANDROID_QEMU_ARM_DEVICES}")

android_target_include_directories(android-qemu-deps linux-x86_64 INTERFACE
                                   linux-headers)
android_target_include_directories(android-qemu-deps linux-aarch64 INTERFACE
                                   linux-headers)
android_target_compile_options(
  android-qemu-deps
  linux-x86_64
  INTERFACE
  # Emit extra code to check for buffer overflows, such as stack smashing
  # attacks. This is done by adding a guard variable to functions with
  # vulnerable objects. This includes functions that call alloca, and functions
  # with buffers larger than 8 bytes. The guards are initialized when a function
  # is entered and then checked when the function exits. If a guard check fails,
  # an error message is printed and the program exits. Like -fstack-protector
  # but includes additional functions to be protected — those that have local
  # array definitions, or have references to local frame addresses.
  "-fstack-protector-strong"
  "-Wno-address-of-packed-member"
  "-Wno-format-security"
  "-Wno-initializer-overrides"
  "-Wno-tautological-compare"
  "-Wno-tautological-pointer-compare"
  "-Wno-unused-variable"
  "-Wno-implicit-int-float-conversion"
  "-Wno-reorder-init-list")
android_target_link_libraries(android-qemu-deps linux-x86_64 INTERFACE
                              "${TCMALLOC_LIBRARIES} -lutil")
android_target_link_libraries(android-qemu-deps linux-aarch64 INTERFACE
                              "${TCMALLOC_LIBRARIES} -lutil")
android_target_link_libraries(android-qemu-deps darwin-x86_64 INTERFACE
                              "-framework CoreAudio")
android_target_link_libraries(android-qemu-deps windows_msvc-x86_64 INTERFACE
                              android-emu-base)

set(ANDROID_AUTOGEN "qemu2-auto-generated")
if(LINUX_AARCH64)
  set(ASM_DIR_TARGET "../linux-headers/asm-arm64")
else()
  set(ASM_DIR_TARGET "../linux-headers/asm-x86")
endif()

set(NEED_RELINK_ASM FALSE)
if(EXISTS ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/asm)
  # check the link is correct
  if(LINUX_AARCH64)
    if(EXISTS ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/asm/unistd_x32.h)
      set(NEED_RELINK_ASM TRUE)
    endif()
  else()
    if(NOT EXISTS ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/asm/unistd_x32.h)
      set(NEED_RELINK_ASM TRUE)
    endif()
  endif()
else()
  set(NEED_RELINK_ASM TRUE)
endif()

if(NEED_RELINK_ASM)
  message("create_symlink ${ASM_DIR_TARGET} ${ANDROID_AUTOGEN}/asm")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${ASM_DIR_TARGET}"
            "${ANDROID_AUTOGEN}/asm" WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR})
endif()

set(ANDROID_QEMU2_TOP_DIR ${ANDROID_QEMU2_TOP_DIR})
include(cmake-main.${ANDROID_TARGET_TAG}.inc)

# Once we have a newer version of cmake we can generate the object library in a
# platform independent way b/121393952
android_add_library(
  TARGET qemu2-common
  LICENSE "GPL-2.0-only"
  LIBNAME
    qemu
    URL
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      ${qemu2-shared_generated_sources} ${qemu2-shared_sources})
target_compile_definitions(qemu2-common PRIVATE "-DPOISON_CONFIG_ANDROID")
target_compile_options(qemu2-common PRIVATE "-Wno-string-plus-int"
                                            "-Wno-return-type")
target_link_libraries(qemu2-common PUBLIC android-qemu-deps emulator-libusb)

if(NOT OPTION_AEMU_LIBS_ONLY)

  # Not allowed to have libraries without source files.
  android_add_library(
    TARGET headless-paaudio
    LICENSE "GPL-2.0-only"
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE"
    SRC # cmake-format: sortable
        audio/paaudio-headless-impl-placeholder.c
    LINUX audio/paaudio-headless-impl.c)
  if(LINUX_AARCH64)
    target_include_directories(
      headless-paaudio
      PUBLIC
        ${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qemu-android-deps/linux-aarch64/include
    )
  endif()

  android_add_interface(
    TARGET android-qemu-deps-headful
    LICENSE "GPL-2.0-only"
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE")
  android_target_link_libraries(android-qemu-deps-headful linux-x86_64
                                INTERFACE "-lpulse")

  android_add_interface(
    TARGET android-qemu-deps-headless
    LICENSE "GPL-2.0-only"
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE")
  android_target_link_libraries(android-qemu-deps-headless linux-x86_64
                                INTERFACE headless-paaudio)

  android_target_link_libraries(android-qemu-deps-headless linux-aarch64
                                INTERFACE headless-paaudio)

  android_add_library(
    TARGET libqemu2-util
    LICENSE "GPL-2.0-only"
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE"
    SRC # cmake-format: sortable
        ${libqemuutil_generated_sources} ${libqemuutil_sources})
  target_link_libraries(libqemu2-util PRIVATE android-qemu-deps FDT)
  android_target_link_libraries(
    libqemu2-util windows PRIVATE psapi::psapi winmm::winmm dxguid::dxguid)
  target_compile_definitions(libqemu2-util PRIVATE "-DPOISON_CONFIG_ANDROID")

  android_add_library(
    TARGET libqemustub
    LICENSE "GPL-2.0-only"
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE"
    SRC # cmake-format: sortable
        ${libqemustub_sources} ${libqemuutil_generated_sources})
  target_link_libraries(libqemustub PRIVATE android-qemu-deps)
  target_compile_definitions(libqemustub PRIVATE "-DLIBQEMUSTUB")

  # Qemu-img
  android_add_executable(
    TARGET qemu-img INSTALL .
    LICENSE GPL-2.0-only
    LIBNAME
      qemu
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSE"
    SRC # cmake-format: sortable
        ${qemu-img_sources} block/qcow2-bitmap.c
    LINUX scsi/pr-manager.c)
  target_link_libraries(qemu-img PRIVATE libqemu2-util libqemustub
                                         android-qemu-deps android-emu-base)

  # Create the qemu targets.
  if(NOT LINUX_AARCH64)
    android_add_qemu_executable(
      x86_64 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_headless_executable(
      x86_64 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
  endif()
  android_add_qemu_executable(
    aarch64 "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")
  android_add_qemu_headless_executable(
    aarch64 "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")

  if(NOT OPTION_MINBUILD)
    android_add_qemu_executable(
      i386 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_executable(
      armel "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_headless_executable(
      i386 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_headless_executable(
      armel "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")
  endif()

  # and the upstream qemu targets, if desired.
  if(QEMU_UPSTREAM)
    message(STATUS "Building upstream qemu.")
    android_add_qemu_upstream_executable(
      i386 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_upstream_executable(
      x86_64 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_upstream_executable(
      aarch64 "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")
    android_add_qemu_upstream_executable(
      armel "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}")
  endif()

  # The tests currently work on linux only.
  if(ANDROID_TARGET_TAG MATCHES "linux.*")
    include(qemu-test.cmake)
  endif()

endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/target.tag "${ANDROID_TARGET_TAG}")

# Construct licenses for all targets, this should always be the last thing we
# do.
finalize_all_licenses()
