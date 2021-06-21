# We might be running in cross compile build, so setup the required paths and includes..
cmake_minimum_required(VERSION 3.5)
project(m4)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

  add_cxx_flag("-std=c++17")
endif()


# Need for possible cross compilationr
include(android)
include(prebuilts)

# Porting M4 to windows is not worth it, we will use a prebuilt.
if(WINDOWS)
  message(STATUS "Using prebuilt on win32")
  get_filename_component(
    M4_EXECUTABLE
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/m4/windows_msvc-x86_64/bin/m4.exe"
    ABSOLUTE)
  add_executable(m4 IMPORTED GLOBAL)
  set_property(TARGET m4 PROPERTY IMPORTED_LOCATION ${M4_EXECUTABLE})
  return()
endif()

set(M4_ROOT ${ANDROID_QEMU2_TOP_DIR}/../m4/)
android_add_executable(
  TARGET
    m4
    URL
    "https://android.googlesource.com/platform/external/bison/+/refs/heads/master"
  REPO "${M4_ROOT}"
  NOTICE "REPO/NOTICE"
  LICENSE "GPL3"
  SRC ${M4_ROOT}/lib/asnprintf.c
      ${M4_ROOT}/lib/basename-lgpl.c
      ${M4_ROOT}/lib/basename.c
      ${M4_ROOT}/lib/binary-io.c
      ${M4_ROOT}/lib/c-ctype.c
      ${M4_ROOT}/lib/c-stack.c
      ${M4_ROOT}/lib/c-strcasecmp.c
      ${M4_ROOT}/lib/c-strncasecmp.c
      ${M4_ROOT}/lib/clean-temp.c
      ${M4_ROOT}/lib/cloexec.c
      ${M4_ROOT}/lib/close-stream.c
      ${M4_ROOT}/lib/closein.c
      ${M4_ROOT}/lib/closeout.c
      ${M4_ROOT}/lib/dirname-lgpl.c
      ${M4_ROOT}/lib/dirname.c
      ${M4_ROOT}/lib/dup-safer-flag.c
      ${M4_ROOT}/lib/dup-safer.c
      ${M4_ROOT}/lib/execute.c
      ${M4_ROOT}/lib/exitfail.c
      ${M4_ROOT}/lib/fatal-signal.c
      ${M4_ROOT}/lib/fclose.c
      ${M4_ROOT}/lib/fd-hook.c
      ${M4_ROOT}/lib/fd-safer-flag.c
      ${M4_ROOT}/lib/fd-safer.c
      ${M4_ROOT}/lib/fflush.c
      ${M4_ROOT}/lib/filenamecat-lgpl.c
      ${M4_ROOT}/lib/filenamecat.c
      ${M4_ROOT}/lib/fopen-safer.c
      ${M4_ROOT}/lib/fpurge.c
      ${M4_ROOT}/lib/freadahead.c
      ${M4_ROOT}/lib/freading.c
      ${M4_ROOT}/lib/fseek.c
      ${M4_ROOT}/lib/fseeko.c
      ${M4_ROOT}/lib/getprogname.c
      ${M4_ROOT}/lib/gl_avltree_oset.c
      ${M4_ROOT}/lib/gl_linkedhash_list.c
      ${M4_ROOT}/lib/gl_list.c
      ${M4_ROOT}/lib/gl_oset.c
      ${M4_ROOT}/lib/gl_xlist.c
      ${M4_ROOT}/lib/gl_xoset.c
      ${M4_ROOT}/lib/glthread/lock.c
      ${M4_ROOT}/lib/glthread/threadlib.c
      ${M4_ROOT}/lib/glthread/tls.c
      ${M4_ROOT}/lib/hard-locale.c
      ${M4_ROOT}/lib/localcharset.c
      ${M4_ROOT}/lib/malloca.c
      ${M4_ROOT}/lib/math.c
      ${M4_ROOT}/lib/memchr2.c
      ${M4_ROOT}/lib/mkstemp-safer.c
      ${M4_ROOT}/lib/obstack.c
      ${M4_ROOT}/lib/pipe-safer.c
      ${M4_ROOT}/lib/pipe2-safer.c
      ${M4_ROOT}/lib/pipe2.c
      ${M4_ROOT}/lib/printf-args.c
      ${M4_ROOT}/lib/printf-frexp.c
      ${M4_ROOT}/lib/printf-frexpl.c
      ${M4_ROOT}/lib/printf-parse.c
      ${M4_ROOT}/lib/progname.c
      ${M4_ROOT}/lib/quotearg.c
      ${M4_ROOT}/lib/sig-handler.c
      ${M4_ROOT}/lib/spawn-pipe.c
      ${M4_ROOT}/lib/stripslash.c
      ${M4_ROOT}/lib/tempname.c
      ${M4_ROOT}/lib/tmpdir.c
      ${M4_ROOT}/lib/unistd.c
      ${M4_ROOT}/lib/vasnprintf.c
      ${M4_ROOT}/lib/verror.c
      ${M4_ROOT}/lib/version-etc-fsf.c
      ${M4_ROOT}/lib/version-etc.c
      ${M4_ROOT}/lib/wait-process.c
      ${M4_ROOT}/lib/wctype-h.c
      ${M4_ROOT}/lib/xalloc-die.c
      ${M4_ROOT}/lib/xasprintf.c
      ${M4_ROOT}/lib/xmalloc.c
      ${M4_ROOT}/lib/xmalloca.c
      ${M4_ROOT}/lib/xprintf.c
      ${M4_ROOT}/lib/xsize.c
      ${M4_ROOT}/lib/xstrndup.c
      ${M4_ROOT}/lib/xvasprintf.c
      ${M4_ROOT}/src/builtin.c
      ${M4_ROOT}/src/debug.c
      ${M4_ROOT}/src/eval.c
      ${M4_ROOT}/src/format.c
      ${M4_ROOT}/src/freeze.c
      ${M4_ROOT}/src/input.c
      ${M4_ROOT}/src/m4.c
      ${M4_ROOT}/src/macro.c
      ${M4_ROOT}/src/output.c
      ${M4_ROOT}/src/path.c
      ${M4_ROOT}/src/symtab.c
  DARWIN ${M4_ROOT}/lib/asprintf.c
         ${M4_ROOT}/lib/canonicalize-lgpl.c
         ${M4_ROOT}/lib/error.c
         ${M4_ROOT}/lib/fpending.c
         ${M4_ROOT}/lib/getopt.c
         ${M4_ROOT}/lib/getopt1.c
         ${M4_ROOT}/lib/isnanf.c
         ${M4_ROOT}/lib/isnanl.c
         ${M4_ROOT}/lib/lstat.c
         ${M4_ROOT}/lib/open.c
         ${M4_ROOT}/lib/rawmemchr.c
         ${M4_ROOT}/lib/readlink.c
         ${M4_ROOT}/lib/regex.c
         ${M4_ROOT}/lib/rename.c
         ${M4_ROOT}/lib/secure_getenv.c
         ${M4_ROOT}/lib/stat.c
         ${M4_ROOT}/lib/strchrnul.c
         ${M4_ROOT}/lib/strerror-override.c
         ${M4_ROOT}/lib/strerror.c
         ${M4_ROOT}/lib/strstr.c
         ${M4_ROOT}/lib/vasprintf.c)

target_include_directories(m4 PRIVATE ${M4_ROOT}/src ${M4_ROOT}/lib)
android_target_include_directories(m4 darwin-x86_64 PRIVATE
                                   ${M4_ROOT}/soong/darwin)
android_target_include_directories(m4 darwin-x86_64 PRIVATE
                                   ${M4_ROOT}/soong/darwin)

