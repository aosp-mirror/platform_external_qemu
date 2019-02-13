#===============================================================================
# Setup Project
#===============================================================================

cmake_minimum_required(VERSION 3.5)

# Add path for custom modules
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
get_filename_component(ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/../libunwind_llvm/cmake/" ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")

# Rely on llvm-config.
set(CONFIG_OUTPUT)
get_clang_version(CLANG_VER)
# Configure the clang dir:
if(APPLE OR ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
set(LLVM_CONFIG_PATH "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/darwin-x86/${CLANG_VER}/bin/llvm-config")
elseif(UNIX OR ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
set(LLVM_CONFIG_PATH "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/linux-x86/${CLANG_VER}/bin/llvm-config")
#not working for windows build yet.
elseif(ANDROID_TARGET_TAG MATCHES "windows.*")
set(LLVM_CONFIG_PATH "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/windows-x86/${CLANG_VER}/bin/llvm-config.exe")
endif()

if(NOT EXISTS "${LLVM_CONFIG_PATH}")
  message(FATAL_ERROR
    "$${LLVM_CONFIG_PATH} does not exists, will not be able to compile properly.")
else()
  message(STATUS "Found LLVM_CONFIG_PATH: ${LLVM_CONFIG_PATH}")
endif()

project(libunwind)

set(CONFIG_COMMAND ${LLVM_CONFIG_PATH} "--includedir" "--prefix" "--src-root")
execute_process(COMMAND ${CONFIG_COMMAND}
                RESULT_VARIABLE HAD_ERROR
                OUTPUT_VARIABLE CONFIG_OUTPUT)
if (NOT HAD_ERROR)
  string(REGEX REPLACE "[ \t]*[\r\n]+[ \t]*" ";"
         CONFIG_OUTPUT ${CONFIG_OUTPUT})
else()
  string(REPLACE ";" " " CONFIG_COMMAND_STR "${CONFIG_COMMAND}")
  message(STATUS "${CONFIG_COMMAND_STR}")
  message(FATAL_ERROR "llvm-config failed with status ${HAD_ERROR}")
endif()

list(GET CONFIG_OUTPUT 0 INCLUDE_DIR)
list(GET CONFIG_OUTPUT 1 LLVM_OBJ_ROOT)
list(GET CONFIG_OUTPUT 2 MAIN_SRC_DIR)

set(LLVM_INCLUDE_DIR ${INCLUDE_DIR} CACHE PATH "Path to llvm/include")
set(LLVM_BINARY_DIR ${LLVM_OBJ_ROOT} CACHE PATH "Path to LLVM build tree")
set(LLVM_MAIN_SRC_DIR ${MAIN_SRC_DIR} CACHE PATH "Path to LLVM source tree")
set(LLVM_LIT_PATH "${LLVM_PATH}/utils/lit/lit.py")

# --cmakedir is supported since llvm r291218 (4.0 release)
execute_process(
  COMMAND ${LLVM_CONFIG_PATH} --cmakedir
  RESULT_VARIABLE HAD_ERROR
  OUTPUT_VARIABLE CONFIG_OUTPUT
  ERROR_QUIET)
if(NOT HAD_ERROR)
  string(STRIP "${CONFIG_OUTPUT}" LLVM_CMAKE_PATH)
else()
  set(LLVM_CMAKE_PATH
      "${LLVM_BINARY_DIR}/lib${LLVM_LIBDIR_SUFFIX}/cmake/llvm")
endif()

if (EXISTS ${LLVM_CMAKE_PATH})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_PATH}")
include("${LLVM_CMAKE_PATH}/AddLLVM.cmake")
include("${LLVM_CMAKE_PATH}/HandleLLVMOptions.cmake")
else()
message(FATAL_ERROR "Not found: ${LLVM_CMAKE_PATH}")
endif()

set(PACKAGE_NAME libunwind)
set(PACKAGE_VERSION 5.0.0svn)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "llvm-bugs@lists.llvm.org")

set(LLVM_INCLUDE_TESTS OFF)

#if (EXISTS ${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py)
#set(LLVM_LIT ${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py)
#else()
# Seek installed Lit.
#find_program(LLVM_LIT "lit.py" ${LLVM_MAIN_SRC_DIR}/utils/lit
#             DOC "Path to lit.py")
#endif()

#if (LLVM_LIT)
# Define the default arguments to use with 'lit', and an option for the user
# to override.
#set(LIT_ARGS_DEFAULT "-sv")
#if (MSVC OR XCODE)
#  set(LIT_ARGS_DEFAULT "${LIT_ARGS_DEFAULT} --no-progress-bar")
#endif()
#set(LLVM_LIT_ARGS "${LIT_ARGS_DEFAULT}" CACHE STRING "Default options for lit")

# On Win32 hosts, provide an option to specify the path to the GnuWin32 tools.
#if (WIN32 AND NOT CYGWIN)
#  set(LLVM_LIT_TOOLS_DIR "" CACHE PATH "Path to GnuWin32 tools")
#endif()
#else()
#set(LLVM_INCLUDE_TESTS OFF)
#endif()



#===============================================================================
# Setup CMake Options
#===============================================================================
include(HandleCompilerRT)

# Define options.
option(LIBUNWIND_BUILD_32_BITS "Build 32 bit libunwind" ${LLVM_BUILD_32_BITS})
option(LIBUNWIND_ENABLE_ASSERTIONS "Enable assertions independent of build mode." OFF)
option(LIBUNWIND_ENABLE_PEDANTIC "Compile with pedantic enabled." ON)
option(LIBUNWIND_ENABLE_WERROR "Fail and stop if a warning is triggered." OFF)
option(LIBUNWIND_ENABLE_SHARED "Build libunwind as a shared library." OFF)
option(LIBUNWIND_ENABLE_STATIC "Build libunwind as a static library." ON)
option(LIBUNWIND_ENABLE_CROSS_UNWINDING "Enable cross-platform unwinding support." OFF)
option(LIBUNWIND_ENABLE_ARM_WMMX "Enable unwinding support for ARM WMMX registers." OFF)
option(LIBUNWIND_ENABLE_THREADS "Build libunwind with threading support." ON)
option(LIBUNWIND_USE_COMPILER_RT "Use compiler-rt instead of libgcc" OFF)
option(LIBUNWIND_INCLUDE_DOCS "Build the libunwind documentation." OFF)

set(LIBUNWIND_LIBDIR_SUFFIX "${LLVM_LIBDIR_SUFFIX}" CACHE STRING
    "Define suffix of library directory name (32/64)")
option(LIBUNWIND_INSTALL_LIBRARY "Install the libunwind library." OFF)
set(LIBUNWIND_TARGET_TRIPLE "" CACHE STRING "Target triple for cross compiling.")
set(LIBUNWIND_GCC_TOOLCHAIN "" CACHE PATH "GCC toolchain for cross compiling.")
set(LIBUNWIND_SYSROOT "" CACHE PATH "Sysroot for cross compiling.")

if (NOT LIBUNWIND_ENABLE_SHARED AND NOT LIBUNWIND_ENABLE_STATIC)
  message(FATAL_ERROR "libunwind must be built as either a shared or static library.")
endif()

# Check that we can build with 32 bits if requested.
if (CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
  if (LIBUNWIND_BUILD_32_BITS AND NOT LLVM_BUILD_32_BITS) # Don't duplicate the output from LLVM
    message(STATUS "Building 32 bits executables and libraries.")
  endif()
elseif(LIBUNWIND_BUILD_32_BITS)
  message(FATAL_ERROR "LIBUNWIND_BUILD_32_BITS=ON is not supported on this platform.")
endif()

#===============================================================================
# Configure System
#===============================================================================

# Add path for custom modules
set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    ${CMAKE_MODULE_PATH})

set(LIBUNWIND_COMPILER    ${CMAKE_CXX_COMPILER})
set(LIBUNWIND_SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBUNWIND_BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR})
if (LLVM_LIBRARY_OUTPUT_INTDIR)
  set(LIBUNWIND_LIBRARY_DIR ${LLVM_LIBRARY_OUTPUT_INTDIR})
else()
  set(LIBUNWIND_LIBRARY_DIR ${CMAKE_BINARY_DIR}/lib${LIBUNWIND_LIBDIR_SUFFIX})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBUNWIND_LIBRARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBUNWIND_LIBRARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIBUNWIND_LIBRARY_DIR})

set(LIBUNWIND_INSTALL_PREFIX "" CACHE STRING
    "Define libunwind destination prefix.")

if (NOT LIBUNWIND_INSTALL_PREFIX MATCHES "^$|.*/")
  message(FATAL_ERROR "LIBUNWIND_INSTALL_PREFIX has to end with \"/\".")
endif()

set(LIBUNWIND_C_FLAGS "")
set(LIBUNWIND_CXX_FLAGS "")
set(LIBUNWIND_COMPILE_FLAGS "")
set(LIBUNWIND_LINK_FLAGS "")

# Get required flags.
macro(append_if list condition var)
  if (${condition})
    list(APPEND ${list} ${var})
  endif()
endmacro()

macro(add_target_flags_if condition var)
  if (${condition})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${var}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${var}")
    list(APPEND LIBUNWIND_COMPILE_FLAGS ${var})
    list(APPEND LIBUNWIND_LINK_FLAGS ${var})
  endif()
endmacro()

add_target_flags_if(LIBUNWIND_BUILD_32_BITS "-m32")
add_target_flags_if(LIBUNWIND_TARGET_TRIPLE
          "--target=${LIBUNWIND_TARGET_TRIPLE}")
add_target_flags_if(LIBUNWIND_GCC_TOOLCHAIN
          "--gcc-toolchain=${LIBUNWIND_GCC_TOOLCHAIN}")
add_target_flags_if(LIBUNWIND_SYSROOT
          "--sysroot=${LIBUNWIND_SYSROOT}")

if (LIBUNWIND_TARGET_TRIPLE)
  set(TARGET_TRIPLE "${LIBUNWIND_TARGET_TRIPLE}")
endif()

# Configure compiler.
include(config-ix)

if (LIBUNWIND_USE_COMPILER_RT)
  list(APPEND LIBUNWIND_LINK_FLAGS "-rtlib=compiler-rt")
endif()

#===============================================================================
# Setup Compiler Flags
#===============================================================================

append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WERROR_FLAG -Werror=return-type)

# Get warning flags
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_W_FLAG -W)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WALL_FLAG -Wall)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WCHAR_SUBSCRIPTS_FLAG -Wchar-subscripts)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WCONVERSION_FLAG -Wconversion)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WMISMATCHED_TAGS_FLAG -Wmismatched-tags)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WMISSING_BRACES_FLAG -Wmissing-braces)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WNEWLINE_EOF_FLAG -Wnewline-eof)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WNO_UNUSED_FUNCTION_FLAG -Wno-unused-function)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSHADOW_FLAG -Wshadow)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSHORTEN_64_TO_32_FLAG -Wshorten-64-to-32)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSIGN_COMPARE_FLAG -Wsign-compare)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSIGN_CONVERSION_FLAG -Wsign-conversion)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSTRICT_ALIASING_FLAG -Wstrict-aliasing=2)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WSTRICT_OVERFLOW_FLAG -Wstrict-overflow=4)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WUNUSED_PARAMETER_FLAG -Wunused-parameter)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WUNUSED_VARIABLE_FLAG -Wunused-variable)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WWRITE_STRINGS_FLAG -Wwrite-strings)
append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WUNDEF_FLAG -Wundef)

if (LIBUNWIND_ENABLE_WERROR)
  append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WERROR_FLAG -Werror)
  append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WX_FLAG -WX)
else()
  append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_WNO_ERROR_FLAG -Wno-error)
  append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_NO_WX_FLAG -WX-)
endif()

if (LIBUNWIND_ENABLE_PEDANTIC)
  append_if(LIBUNWIND_COMPILE_FLAGS LIBUNWIND_HAS_PEDANTIC_FLAG -pedantic)
endif()

# Get feature flags.
# Exceptions
# Catches C++ exceptions only and tells the compiler to assume that extern C
# functions never throw a C++ exception.
append_if(LIBUNWIND_CXX_FLAGS LIBUNWIND_HAS_FSTRICT_ALIASING_FLAG -fstrict-aliasing)
append_if(LIBUNWIND_CXX_FLAGS LIBUNWIND_HAS_EHSC_FLAG -EHsc)

append_if(LIBUNWIND_C_FLAGS LIBUNWIND_HAS_FUNWIND_TABLES -funwind-tables)

# Assert
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if (LIBUNWIND_ENABLE_ASSERTIONS)
  # MSVC doesn't like _DEBUG on release builds. See PR 4379.
  if (NOT MSVC)
    list(APPEND LIBUNWIND_COMPILE_FLAGS -D_DEBUG)
  endif()

  # On Release builds cmake automatically defines NDEBUG, so we
  # explicitly undefine it:
  if (uppercase_CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    list(APPEND LIBUNWIND_COMPILE_FLAGS -UNDEBUG)
  endif()
else()
  if (NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    list(APPEND LIBUNWIND_COMPILE_FLAGS -DNDEBUG)
  endif()
endif()

# Cross-unwinding
if (NOT LIBUNWIND_ENABLE_CROSS_UNWINDING)
  list(APPEND LIBUNWIND_COMPILE_FLAGS -D_LIBUNWIND_IS_NATIVE_ONLY)
endif()

# Threading-support
if (NOT LIBUNWIND_ENABLE_THREADS)
  list(APPEND LIBUNWIND_COMPILE_FLAGS -D_LIBUNWIND_HAS_NO_THREADS)
endif()

# This is the _ONLY_ place where add_definitions is called.
if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Disable DLL annotations on Windows for static builds.
if (WIN32 AND LIBUNWIND_ENABLE_STATIC AND NOT LIBUNWIND_ENABLE_SHARED)
  add_definitions(-D_LIBUNWIND_DISABLE_VISIBILITY_ANNOTATIONS)
endif()

#===============================================================================
# Setup Source Code
#===============================================================================

include_directories("${ADD_PATH}/include")

find_path(
  LIBUNWIND_LIBCXX_INCLUDES_INTERNAL
  __libcpp_version
  PATHS ${LLVM_MAIN_SRC_DIR}/projects/libcxx/include
        ${LLVM_MAIN_SRC_DIR}/runtimes/libcxx/include
  NO_DEFAULT_PATH
)
if ((NOT LIBUNWIND_STANDALONE_BUILD OR HAVE_LIBCXX) AND
    IS_DIRECTORY "${LIBUNWIND_LIBCXX_INCLUDES_INTERNAL}")
  set(LIBUNWIND_CXX_INCLUDE_PATHS_DEFAULT "${LIBUNWIND_LIBCXX_INCLUDES_INTERNAL}")
endif()

set(LIBUNWIND_CXX_INCLUDE_PATHS "${LIBUNWIND_CXX_INCLUDE_PATHS_DEFAULT}" CACHE PATH
    "Paths to C++ header directories separated by ';'.")

if (NOT LIBUNWIND_CXX_INCLUDE_PATHS STREQUAL "")
  list(APPEND LIBUNWIND_CXX_FLAGS -nostdinc++)
  include_directories("${LIBUNWIND_CXX_INCLUDE_PATHS}")
endif()

# Get sources
set(LIBUNWIND_SOURCE_DIR 
    "${ANDROID_QEMU2_TOP_DIR}/../libunwind_llvm/src")
set(LIBUNWIND_CXX_SOURCES
    "${LIBUNWIND_SOURCE_DIR}/libunwind.cpp"
    "${LIBUNWIND_SOURCE_DIR}/Unwind-EHABI.cpp")
append_if(LIBUNWIND_CXX_SOURCES APPLE "${LIBUNWIND_SOURCE_DIR}/Unwind_AppleExtras.cpp")

set(LIBUNWIND_C_SOURCES
    "${LIBUNWIND_SOURCE_DIR}/UnwindLevel1.c"
    "${LIBUNWIND_SOURCE_DIR}/UnwindLevel1-gcc-ext.c"
    "${LIBUNWIND_SOURCE_DIR}/Unwind-sjlj.c")
set_source_files_properties(${LIBUNWIND_C_SOURCES}
                            PROPERTIES
                              COMPILE_FLAGS "-std=c99")

set(LIBUNWIND_ASM_SOURCES
    "${LIBUNWIND_SOURCE_DIR}/UnwindRegistersRestore.S"
    "${LIBUNWIND_SOURCE_DIR}/UnwindRegistersSave.S")
set_source_files_properties(${LIBUNWIND_ASM_SOURCES}
                            PROPERTIES
                              LANGUAGE C)

set(LIBUNWIND_HEADERS
    "${LIBUNWIND_SOURCE_DIR}/AddressSpace.hpp"
    "${LIBUNWIND_SOURCE_DIR}/assembly.h"
    "${LIBUNWIND_SOURCE_DIR}/CompactUnwinder.hpp"
    "${LIBUNWIND_SOURCE_DIR}/config.h"
    "${LIBUNWIND_SOURCE_DIR}/dwarf2.h"
    "${LIBUNWIND_SOURCE_DIR}/DwarfInstructions.hpp"
    "${LIBUNWIND_SOURCE_DIR}/DwarfParser.hpp"
    "${LIBUNWIND_SOURCE_DIR}/libunwind_ext.h"
    "${LIBUNWIND_SOURCE_DIR}/Registers.hpp"
    "${LIBUNWIND_SOURCE_DIR}/RWMutex.hpp"
    "${LIBUNWIND_SOURCE_DIR}/UnwindCursor.hpp"
    "${LIBUNWIND_SOURCE_DIR}/../include/libunwind.h"
    "${LIBUNWIND_SOURCE_DIR}/../include/unwind.h")

append_if(LIBUNWIND_HEADERS APPLE
          "${LIBUNWIND_SOURCE_DIR}/../include/mach-o/compact_unwind_encoding.h")

if (MSVC_IDE)
  # Force them all into the headers dir on MSVC, otherwise they end up at
  # project scope because they don't have extensions.
  source_group("Header Files" FILES ${LIBUNWIND_HEADERS})
endif()

set(LIBUNWIND_SOURCES
    ${LIBUNWIND_CXX_SOURCES}
    ${LIBUNWIND_C_SOURCES}
    ${LIBUNWIND_ASM_SOURCES})

# Generate library list.
set(libraries ${LIBUNWINDCXX_ABI_LIBRARIES})
append_if(libraries LIBUNWIND_HAS_C_LIB c)
append_if(libraries LIBUNWIND_HAS_DL_LIB dl)
if (LIBUNWIND_ENABLE_THREADS)
  append_if(libraries LIBUNWIND_HAS_PTHREAD_LIB pthread)
endif()

# Setup flags.
append_if(LIBUNWIND_CXX_FLAGS LIBUNWIND_HAS_NO_RTTI_FLAG -fno-rtti)

append_if(LIBUNWIND_LINK_FLAGS LIBUNWIND_HAS_NODEFAULTLIBS_FLAG -nodefaultlibs)

if (LIBUNWIND_HAS_NO_EXCEPTIONS_FLAG AND LIBUNWIND_HAS_FUNWIND_TABLES)
  list(APPEND LIBUNWIND_COMPILE_FLAGS -fno-exceptions)
  list(APPEND LIBUNWIND_COMPILE_FLAGS -funwind-tables)

elseif (LIBUNWIND_ENABLE_SHARED)
  message(FATAL_ERROR
          "Compiler doesn't support generation of unwind tables if exception "
          "support is disabled.  Building libunwind DSO with runtime dependency "
          "on C++ ABI library is not supported.")
endif()

if (APPLE)
  list(APPEND LIBUNWIND_COMPILE_FLAGS "-U__STRICT_ANSI__")
  list(APPEND LIBUNWIND_LINK_FLAGS
       "-compatibility_version 1"
       "-install_name /usr/lib/libunwind.1.dylib")

  if (CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "10.6")
    list(APPEND LIBUNWIND_LINK_FLAGS
         "-current_version ${LIBUNWIND_VERSION}"
         "/usr/lib/libSystem.B.dylib")
  endif ()
endif ()

string(REPLACE ";" " " LIBUNWIND_COMPILE_FLAGS "${LIBUNWIND_COMPILE_FLAGS}")
string(REPLACE ";" " " LIBUNWIND_CXX_FLAGS "${LIBUNWIND_CXX_FLAGS}")
string(REPLACE ";" " " LIBUNWIND_C_FLAGS "${LIBUNWIND_C_FLAGS}")
string(REPLACE ";" " " LIBUNWIND_LINK_FLAGS "${LIBUNWIND_LINK_FLAGS}")
set_property(SOURCE ${LIBUNWIND_CXX_SOURCES}
             APPEND_STRING PROPERTY COMPILE_FLAGS " ${CMAKE_CXX_FLAGS} ${LIBUNWIND_CXX_FLAGS}")
set_property(SOURCE ${LIBUNWIND_C_SOURCES}
             APPEND_STRING PROPERTY COMPILE_FLAGS " ${CMAKE_C_FLAGS} ${LIBUNWIND_C_FLAGS}")

# Add a object library that contains the compiled source files.
add_library(unwind_objects OBJECT ${LIBUNWIND_SOURCES} ${LIBUNWIND_HEADERS})

set_target_properties(unwind_objects
                      PROPERTIES
                        COMPILE_FLAGS "${LIBUNWIND_COMPILE_FLAGS}"
                        POSITION_INDEPENDENT_CODE ON)

target_include_directories(unwind_objects PUBLIC ${LIBUNWIND_SOURCE_DIR}/../include)
target_include_directories(unwind_objects PRIVATE ${LIBUNWIND_SOURCE_DIR})

set(LIBUNWIND_TARGETS)

# Build the shared library.
if (LIBUNWIND_ENABLE_SHARED)
  add_library(unwind_shared SHARED $<TARGET_OBJECTS:unwind_objects>)
  target_link_libraries(unwind_shared ${libraries})
  target_include_directories(unwind_shared PUBLIC ${LIBUNWIND_SOURCE_DIR}/../include)
  set_target_properties(unwind_shared
                        PROPERTIES
                          LINK_FLAGS    "${LIBUNWIND_LINK_FLAGS}"
                          OUTPUT_NAME   "unwind"
                          VERSION       "1.0"
                          SOVERSION     "1")
  list(APPEND LIBUNWIND_TARGETS "unwind_shared")
  android_install_shared(unwind_shared)
endif()

# Build the static library.
if (LIBUNWIND_ENABLE_STATIC)
  add_library(unwind_static STATIC $<TARGET_OBJECTS:unwind_objects>)
  target_link_libraries(unwind_static ${libraries})
  target_include_directories(unwind_static PUBLIC ${LIBUNWIND_SOURCE_DIR}/../include)
  set_target_properties(unwind_static
                        PROPERTIES
                          LINK_FLAGS    "${LIBUNWIND_LINK_FLAGS}"
                          OUTPUT_NAME   "unwind")
  list(APPEND LIBUNWIND_TARGETS "unwind_static")
endif()

# Add a meta-target for both libraries.
add_custom_target(unwind DEPENDS ${LIBUNWIND_TARGETS})

if (LIBUNWIND_INSTALL_LIBRARY)
  install(TARGETS ${LIBUNWIND_TARGETS}
    LIBRARY DESTINATION ${LIBUNWIND_INSTALL_PREFIX}lib${LIBUNWIND_LIBDIR_SUFFIX} COMPONENT unwind
    ARCHIVE DESTINATION ${LIBUNWIND_INSTALL_PREFIX}lib${LIBUNWIND_LIBDIR_SUFFIX} COMPONENT unwind)
endif()

if (NOT CMAKE_CONFIGURATION_TYPES AND LIBUNWIND_INSTALL_LIBRARY)
  add_custom_target(install-unwind
    DEPENDS unwind
    COMMAND "${CMAKE_COMMAND}"
            -DCMAKE_INSTALL_COMPONENT=unwind
            -P "${LIBUNWIND_BINARY_DIR}/cmake_install.cmake")
  add_custom_target(install-unwind-stripped
    DEPENDS unwind
    COMMAND "${CMAKE_COMMAND}"
            -DCMAKE_INSTALL_COMPONENT=unwind
            -DCMAKE_INSTALL_DO_STRIP=1
            -P "${LIBUNWIND_BINARY_DIR}/cmake_install.cmake")
endif()

