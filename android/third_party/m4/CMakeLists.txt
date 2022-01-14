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
  LICENSE "GPL3" SOURCE_DIR ${M4_ROOT}
  SRC lib/asnprintf.c
      lib/basename-lgpl.c
      lib/basename.c
      lib/binary-io.c
      lib/c-ctype.c
      lib/c-stack.c
      lib/c-strcasecmp.c
      lib/c-strncasecmp.c
      lib/clean-temp.c
      lib/cloexec.c
      lib/close-stream.c
      lib/closein.c
      lib/closeout.c
      lib/dirname-lgpl.c
      lib/dirname.c
      lib/dup-safer-flag.c
      lib/dup-safer.c
      lib/execute.c
      lib/exitfail.c
      lib/fatal-signal.c
      lib/fclose.c
      lib/fcntl.c
      lib/fd-hook.c
      lib/fd-safer-flag.c
      lib/fd-safer.c
      lib/fflush.c
      lib/filenamecat-lgpl.c
      lib/filenamecat.c
      lib/fopen-safer.c
      lib/fpurge.c
      lib/freadahead.c
      lib/freading.c
      lib/fseek.c
      lib/fseeko.c
      lib/getprogname.c
      lib/gl_avltree_oset.c
      lib/gl_linkedhash_list.c
      lib/gl_list.c
      lib/gl_oset.c
      lib/gl_xlist.c
      lib/gl_xoset.c
      lib/glthread/lock.c
      lib/glthread/threadlib.c
      lib/glthread/tls.c
      lib/hard-locale.c
      lib/localcharset.c
      lib/malloca.c
      lib/math.c
      lib/mbrtowc.c
      lib/memchr2.c
      lib/mkstemp-safer.c
      lib/obstack.c
      lib/pipe-safer.c
      lib/pipe2-safer.c
      lib/pipe2.c
      lib/printf-args.c
      lib/printf-frexp.c
      lib/printf-frexpl.c
      lib/printf-parse.c
      lib/progname.c
      lib/quotearg.c
      lib/sig-handler.c
      lib/spawn-pipe.c
      lib/stripslash.c
      lib/tempname.c
      lib/tmpdir.c
      lib/unistd.c
      lib/vasnprintf.c
      lib/verror.c
      lib/version-etc-fsf.c
      lib/version-etc.c
      lib/wait-process.c
      lib/wctype-h.c
      lib/xalloc-die.c
      lib/xasprintf.c
      lib/xmalloc.c
      lib/xmalloca.c
      lib/xprintf.c
      lib/xsize.c
      lib/xstrndup.c
      lib/xvasprintf.c
      src/builtin.c
      src/debug.c
      src/eval.c
      src/format.c
      src/freeze.c
      src/input.c
      src/m4.c
      src/macro.c
      src/output.c
      src/path.c
      src/symtab.c
  DARWIN lib/asprintf.c
         lib/canonicalize-lgpl.c
         lib/error.c
         lib/fpending.c
         lib/getopt.c
         lib/getopt1.c
         lib/isnanf.c
         lib/isnanl.c
         lib/lstat.c
         lib/open.c
         lib/rawmemchr.c
         lib/readlink.c
         lib/regex.c
         lib/rename.c
         lib/secure_getenv.c
         lib/stat.c
         lib/strchrnul.c
         lib/strerror-override.c
         lib/strerror.c
         lib/strstr.c
         lib/vasprintf.c)

target_include_directories(m4 PRIVATE ${M4_ROOT}/src ${M4_ROOT}/lib)
target_compile_options(m4 PRIVATE -Wno-error)
android_target_include_directories(m4 linux-x86_64 PRIVATE ${M4_ROOT}/soong/linux_glibc)
android_target_include_directories(m4 darwin-x86_64 PRIVATE ${M4_ROOT}/soong/darwin)
