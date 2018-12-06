cmake_minimum_required(VERSION 3.5)

if(MSVC)
  list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/android/build/cmake")
  include("toolchain-windows_msvc-x86_64")
endif()

message(STATUS "Compiling for target: ${ANDROID_TARGET_TAG}")
if(NOT DEFINED ANDROID_TARGET_TAG)
  message(STATUS "ANDROID_TARGET_TAG is undefined [${ANDROID_TARGET_TAG}]")
  message(
    FATAL_ERROR
      "You need to invoke the cmake generator with a proper toolchain from android/build/cmake, pass: -DCMAKE_TOOLCHAIN_FILE=./android/build/cmake/toolchain-xxxx.cmake to cmake"
    )
endif()
message(STATUS "Using toolchain for ${ANDROID_TARGET_TAG}")

include(android)
include(prebuilts)

set(OPTION_CRASHUPLOAD "NONE" CACHE STRING "Destination of crash reports.")
set(OPTION_SDK_TOOLS_REVISION "" CACHE STRING "The tools revision to use, if any")
set(OPTION_SDK_TOOLS_BUILD_NUMBER "" CACHE STRING "The sdk buildnumber to use, if any.")
set(OPTION_ASAN "" CACHE STRING "The list of build sanitizers to user")

set_property(CACHE OPTION_CRASHUPLOAD PROPERTY STRINGS NONE STAGING PROD)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# So we are using some C++14 features..
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Let's bin place everything in the root, with the shared libs in the right place
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archives)

# Feeling courageous? Set this to $ANDROID_SDK_ROOT
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/gradle-release)

# Pthreads from the prebuilts please!
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
prebuilt(Threads)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions("-DANDROID_DEBUG")

  if (MSVC)
    # Clang will be running in cl compatibility mode.
    add_c_flag("/Od /RTC1 /EHsc")
  else()
    add_c_flag("-O0")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if(NOT OPTION_ASAN)
        set(OPTION_ASAN address)
      endif()

      if (NOT MSVC)
        # Build an instrumented version of the code  that generates coverage mapping to enable code coverage analysis
        set(ANDROID_CODE_COVERAGE TRUE)
        add_c_flag("-fcoverage-mapping")
        add_c_flag("-fprofile-instr-generate")
        add_c_flag("-fprofile-arcs")
        add_c_flag("-ftest-coverage")
        add_c_flag("--coverage")
      endif()
    endif()
else()
  add_definitions("-DNDEBUG=1")
  if(ANDROID_TARGET_TAG MATCHES "windows.*")
    add_c_flag("-O2")
  else()
    add_c_flag("-O3")
  endif()
endif()

# Note that clang in windows act as a drop in replacement for cl
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions("-D__STDC_CONSTANT_MACROS" "-D_LIBCPP_VERSION=__GLIBCPP__")

  # Clang goes balistic on our codebase, so let's just suppress all the things.
  add_c_flag("-Wno-mismatched-tags")
  add_c_flag("-Wno-\#warnings")
  add_c_flag("-Wno-unused-variable")
  add_c_flag("-Wno-deprecated-declarations")
  add_c_flag("-Wno-c++14-extensions")
  add_c_flag("-Wno-array-bounds")
  add_c_flag("-Wno-builtin-requires-header")
  add_c_flag("-Wno-constant-conversion")
  add_c_flag("-Wno-deprecated-register")
  add_c_flag("-Wno-extern-c-compat")
  add_c_flag("-Wno-gnu-designator")
  add_c_flag("-Wno-inconsistent-missing-override")
  add_c_flag("-Wno-initializer-overrides")
  add_c_flag("-Wno-invalid-constexpr")
  add_c_flag("-Wno-macro-redefined")
  add_c_flag("-Wno-missing-braces")
  add_c_flag("-Wno-missing-field-initializers")
  add_c_flag("-Wno-parentheses-equality")
  add_c_flag("-Wno-pessimizing-move")
  add_c_flag("-Wno-pointer-bool-conversion")
  add_c_flag("-Wno-return-type-c-linkage")
  add_c_flag("-Wno-self-assign")
  add_c_flag("-Wno-shift-negative-value")
  add_c_flag("-Wno-string-plus-int")
  add_c_flag("-Wno-uninitialized")
  add_c_flag("-Wno-unknown-pragmas")
  add_c_flag("-Wno-unused-command-line-argument")
  add_c_flag("-Wno-unused-const-variable")
  add_c_flag("-Wno-unused-function")
  add_c_flag("-Wno-unused-lambda-capture")
  add_c_flag("-Wno-unused-private-field")
  add_c_flag("-Wno-unused-value")
  add_c_flag("-Wswitch")
endif()

if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_c_flag("-Wno-microsoft-enum-forward-reference")
  add_c_flag("-Wno-address-of-packed-member")
  add_c_flag("-Wno-incompatible-ms-struct")
  add_c_flag("-Wno-c++11-narrowing")
  add_c_flag("-Wno-unknown-argument")
  add_c_flag("-Wno-return-type")
  add_c_flag("-Wno-microsoft-include")
  add_c_flag("-Wno-incompatible-pointer-types")
  add_c_flag("-MD")
endif()

# Disable generation of information about every class with virtual functions for use by the C++ runtime type
# identification features (dynamic_cast and typeid). If you don't use those parts of the language, you can save some
# space by using this flag. Note that exception handling uses the same information, but it will generate it as needed.
# The  dynamic_cast operator can still be used for casts that do not require runtime type information, i.e. casts to
# void * or to unambiguous base classes.
#
# For msvc build, the msvc C++ headers are using RTTI features (typeid) so have cannot disable it.
if(NOT ANDROID_TARGET_TAG MATCHES "windows_msvc.*" AND NOT MSVC)
  add_cxx_flag("-fno-rtti")
endif()

if(NOT MSVC)
  # Compiling with -fno-exceptions disables exceptions support and uses the variant of C++ libraries without exceptions.
  # Use of try, catch, or throw results in an error message. Linking objects that have been compiled with -fno-exceptions
  # automatically selects the libraries without exceptions. You can use the linker option --no_exceptions to diagnose
  # whether the objects being linked contain exceptions. If an exception propagates into a function that has been compiled
  # without exceptions support, then the program terminates.
  add_cxx_flag("-fno-exceptions")
  add_cxx_flag("-Wno-invalid-offsetof")
endif()

# Ensure that <inttypes.h> always defines all interesting macros.
add_definitions("-D__STDC_LIMIT_MACROS=1" "-D__STDC_FORMAT_MACROS=1")

# (From https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply) with GNU_SOURCE, you will get:
#
# * access to lots of nonstandard GNU/Linux extension functions
# * access to traditional functions which were omitted from the POSIX standard (often for good reason, such as being
#   replaced with better alternatives, or being tied to particular legacy implementations)
# * access to low-level functions that cannot be portable, but that you sometimes need for implementing system utilities
#   like mount, ifconfig, etc. broken behavior for lots of POSIX-specified functions, where the GNU folks disagreed with
#   the standards committee on how the functions should behave and decided to do their own thing. As long as you're
#   aware of these things, it should not be a problem to define _GNU_SOURCE, but you should avoid defining it and
#   instead define _POSIX_C_SOURCE=200809L or _XOPEN_SOURCE=700 when possible to ensure that your programs are portable.
add_definitions("-D_GNU_SOURCE=1")

# Enable large-file support (i.e. make off_t a 64-bit value).
add_definitions("-D_FILE_OFFSET_BITS=64" "-D_LARGEFILE_SOURCE")

if(OPTION_ASAN AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
  message(STATUS "Enabling ASAN with: ${OPTION_ASAN}")
  add_c_flag("-fsanitize=${OPTION_ASAN}")
  add_c_flag("-g3")
  if(OPTION_ASAN STREQUAL "address")
    add_c_flag("-fno-omit-frame-pointer")
  endif()
endif()

# We now have configured our base compiler..

# Unit tests please!
enable_testing()

# TODO(jansene): Make these prebuilts. Files on which the emulator (and some unit tests dependes). If a target takes a
# prebuilt_dependency on these, then they will be binplaced in  ${EXECUTABLE_DIRECTORY}/lib/...
set(EMULATOR_FEATURE_DEPENDENCIES "${ANDROID_QEMU2_TOP_DIR}/android/data/advancedFeatures.ini>lib/advancedFeatures.ini"
    "${ANDROID_QEMU2_TOP_DIR}/android/data/advancedFeaturesCanary.ini>lib/advancedFeaturesCanary.ini"
    "${ANDROID_QEMU2_TOP_DIR}/android/data/emu-original-feature-flags.protobuf>lib/emu-original-feature-flags.protobuf")

set(
  SWIFTSHADER_DEPENDENCIES
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_OS_FLAVOR}-x86_64/lib/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}>lib64/gles_swiftshader/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

prebuilt(CURL)
prebuilt(GLIB2)
prebuilt(ZLIB)
prebuilt(FDT)
prebuilt(PIXMAN)
if (NOT ANDROID_TARGET_TAG MATCHES "windows_msvc.*")
  prebuilt(VIRGLRENDERER)
endif()
prebuilt(TCMALLOC)
prebuilt(OpenSSL)

# There is no USB support on windows, so we will not try to resolve it.
if(NOT ANDROID_TARGET_TAG MATCHES "windows.*")
  prebuilt(USB)
endif()

# Get the versions from git (Note that both functions are doing pretty much the same)
get_git_version(ANDROID_QEMU_VERSION)
get_git_sha(ANDROID_QEMU_SHA)

# Generate the qemu-version header.
configure_file(qemu-version.h.in qemu-version.h @ONLY)

# Lets set up a qemu dependency that we all can rely on
android_add_interface(android-qemu-deps)

# This defines all the include directories needed to compile the libqemu libraries. There should be no need to make
# additions here.
target_include_directories(
  android-qemu-deps
  INTERFACE
  tcg
  tcg/i386
  accel/tcg
  include
  disas/libvixl
  ${CMAKE_CURRENT_SOURCE_DIR}
  qemu2-auto-generated
  ../../prebuilts/android-emulator-build/qemu-android-deps/${ANDROID_TARGET_TAG}/include
  android-qemu2-glue/config/${ANDROID_TARGET_TAG}/)

# These are the rules that every qemu component needs, regardless of the platform you are compiling on. Please document
# any flags you set as not everyone is a compiler expert!
target_compile_definitions(android-qemu-deps INTERFACE
                                   # Enable faster migration code when saving RAM to a snapshot
                                   "-DCONFIG_MIGRATION_RAM_SINGLE_ITERATION")
if (NOT ANDROID_TARGET_TAG MATCHES "windows_msvc.*")
    target_compile_definitions(android-qemu-deps INTERFACE
                                   # Enable VIRGL
                                   "-DCONFIG_VIRGL")
endif()


target_compile_options(android-qemu-deps INTERFACE "-Werror=implicit-function-declaration"
                               "-Wno-unused-function"
                               # Do not allow the compiler to assume the strictest aliasing rules
                               # applicable to the language being compiled. For C (and C++), this activates
                               # optimizations based on the type of expressions. In particular, an object
                               # of one type is assumed never to reside at the same address as an object of
                               # a different type, unless the types are almost the same.  Supposedly this
                               # can introduce weird failures esp. on older gcc versions (MINGW)
                               # (See http://thiemonagel.de/2010/01/no-strict-aliasing/)
                               "-fno-strict-aliasing")

if (NOT MSVC)
  target_compile_options(android-qemu-deps INTERFACE
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
endif()
# Make sure we make our dependent libraries available.
target_link_libraries(android-qemu-deps INTERFACE GLIB2::GLIB2 PIXMAN::PIXMAN ZLIB::ZLIB)

if (NOT ANDROID_TARGET_TAG MATCHES "windows_msvc.*")
    # no virglrenderer available for windows_msvc
    target_link_libraries(android-qemu-deps INTERFACE VIRGL::VIRGL)
endif()

# We only support usb on mac/lin
android_target_link_libraries(android-qemu-deps darwin-x86_64 INTERFACE USB::USB)
android_target_link_libraries(android-qemu-deps linux-x86_64 INTERFACE USB::USB)


# Now we add the normal android libs, so we can use them

# Let's not install gtest..
set(INSTALL_GTEST OFF CACHE BOOL "Never install gtest" FORCE)

if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
  # Including the top-level CMakeLists.txt file doesn't work because the Windows SDK
  # library requires C++14 support. So let's just include this manually.

  set(gtest_disable_pthreads ON CACHE BOOL "Disable uses of pthreads in gtest.")
  # All right, we must make sure we don't start adding the /MT flag
  if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)
  endif()
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest/googlemock googlemock)
  if(MSVC)
    # Okay, google test is not happy under msvc compile
    target_compile_definitions(gmock_main PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(gmock PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(gtest PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  endif()
else()
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest googletest)
endif()

if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
    # Don't include the custom TempDir function. It is for Android only.
	target_compile_definitions(gtest PRIVATE
            -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
endif ()

add_subdirectory(android)
add_subdirectory(android-qemu2-glue)

set(ANDROID_QEMU_COMMON_DEVICES "hw/pci/goldfish_address_space.c;hw/i386/pc_piix.c")

# Darwin definitions
set(ANDROID_QEMU_i386_DEVICES_darwin-x86_64 "hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
set(ANDROID_QEMU_arm_DEVICES_darwin-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
android_target_compile_options(android-qemu-deps darwin-x86_64 INTERFACE
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
android_target_compile_definitions(android-qemu-deps darwin-x86_64 INTERFACE "-D_DARWIN_C_SOURCE=1")
android_target_link_libraries(android-qemu-deps darwin-x86_64 INTERFACE "-framework IOKit" "-framework CoreFoundation")

# Windows x86 definitions
set(ANDROID_QEMU_arm_DEVICES_windows-x86 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_i386_DEVICES_windows-x86 "hvf-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
set(ANDROID_QEMU_arm_DEVICES_windows-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_i386_DEVICES_windows-x86_64 "hvf-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
set(ANDROID_QEMU_arm_DEVICES_windows_msvc-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
set(ANDROID_QEMU_i386_DEVICES_windows_msvc-x86_64 "hvf-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
android_target_compile_options(android-qemu-deps
                               windows_msvc
                               INTERFACE
                               "-Werror=implicit-function-declaration"
                               "-Wno-unused-variable"
                               "-mcx16"
                               )
if (NOT MSVC)
  android_target_compile_options(android-qemu-deps windows_msvc INTERFACE "-mms-bitfields")
endif()
android_target_compile_options(android-qemu-deps
                               windows-x86
                               INTERFACE
                               "-Werror=implicit-function-declaration"
                               "-mms-bitfields"
                               "-Wno-unused-variable"
                               "-Wno-unused-but-set-variable"
                               "-Wno-maybe-uninitialized"
                               "-mcx16")
android_target_compile_definitions(android-qemu-deps
                                   windows
                                   INTERFACE
                                   "-DWIN32_LEAN_AND_MEAN"
                                   "-DWINVER=0x601"
                                   "-D_WIN32_WINNT=0x601"
                                   "-D__USE_MINGW_ANSI_STDIO=1")

# Linux settings
set(ANDROID_QEMU_i386_DEVICES_linux-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c;hw/i386/acpi-build.c;${ANDROID_QEMU_COMMON_DEVICES}")
set(ANDROID_QEMU_arm_DEVICES_linux-x86_64 "hvf-stub.c;accel/stubs/hax-stub.c")
android_target_include_directories(android-qemu-deps linux-x86_64 INTERFACE linux-headers)
android_target_compile_options(android-qemu-deps linux-x86_64 INTERFACE
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
android_target_link_libraries(android-qemu-deps linux-x86_64 INTERFACE "${TCMALLOC_LIBRARIES} -lpulse -lutil")
android_target_link_libraries(android-qemu-deps darwin-x86_64 INTERFACE "-framework CoreAudio")

set(ANDROID_AUTOGEN "qemu2-auto-generated")
set(ANDROID_QEMU2_TOP_DIR ${ANDROID_QEMU2_TOP_DIR})
include(cmake-main.${ANDROID_TARGET_TAG}.inc)

set(qemu2-common_src ${qemu2-shared_sources} ${qemu2-shared_generated_sources})
if(MSVC)
  # Once we have a newer version of cmake we can generate the object library in a platform
  # independent way b/121393952
  android_add_object_library(qemu2-common)
else()
  android_add_library(qemu2-common)
endif()
target_compile_definitions(qemu2-common PRIVATE "-DPOISON_CONFIG_ANDROID")
target_link_libraries(qemu2-common PUBLIC android-qemu-deps)

set(libqemu2-util_src ${libqemuutil_sources} ${libqemuutil_generated_sources})
android_add_library(libqemu2-util)
target_link_libraries(libqemu2-util PRIVATE android-qemu-deps FDT)
android_target_link_libraries(libqemu2-util windows PRIVATE psapi::psapi winmm::winmm dxguid::dxguid)
target_compile_definitions(libqemu2-util PRIVATE "-DPOISON_CONFIG_ANDROID")

set(libqemustub_src ${libqemustub_sources} ${libqemuutil_generated_sources})
android_add_library(libqemustub)
target_link_libraries(libqemustub PRIVATE android-qemu-deps)

# Qemu-img
set(qemu-img_src ${qemu-img_sources} block/qcow2-bitmap.c)
set(qemu-img_linux-x86_64_src scsi/pr-manager.c)
android_add_executable(qemu-img)
target_link_libraries(qemu-img PRIVATE libqemu2-util libqemustub android-qemu-deps)
install(TARGETS qemu-img RUNTIME DESTINATION .)

# Create the qemu targets.
android_add_qemu_executable(i386 i386 x86 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}" i386)
android_add_qemu_executable(x86_64 x86_64 x86_64 "${ANDROID_QEMU_i386_DEVICES_${ANDROID_TARGET_TAG}}" i386)
android_add_qemu_executable(aarch64 aarch64 arm64 "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}" arm)
android_add_qemu_executable(armel arm arm "${ANDROID_QEMU_arm_DEVICES_${ANDROID_TARGET_TAG}}" arm)

# The tests currently work on linux only.
if(ANDROID_TARGET_TAG MATCHES "linux.*")
  include(qemu-test.cmake)
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/target.tag "${ANDROID_TARGET_TAG}")
