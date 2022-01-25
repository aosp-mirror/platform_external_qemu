# Builds bison that is included with android, currently only supports linux & macos.

# We might be running in cross compile build, so setup the required paths and
# includes..
cmake_minimum_required(VERSION 3.5)
project(bison)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
endif()

include(android)
include(prebuilts)
set(BISON_ROOT ${ANDROID_QEMU2_TOP_DIR}/../bison/)

android_add_library(
  TARGET
    bison_lib
    URL
    "https://android.googlesource.com/platform/external/bison/+/refs/heads/master"
  REPO "${BISON_ROOT}"
  NOTICE "REPO/NOTICE"
  SOURCE_DIR ${BISON_ROOT}
  LICENSE "GPL3"
  SRC # cmake-format: sortable
      lib/allocator.c
      lib/areadlink.c
      lib/argmatch.c
      lib/asnprintf.c
      lib/basename-lgpl.c
      lib/basename.c
      lib/binary-io.c
      lib/bitrotate.c
      lib/bitset.c
      lib/bitset/array.c
      lib/bitset/list.c
      lib/bitset/stats.c
      lib/bitset/table.c
      lib/bitset/vector.c
      lib/bitsetv.c
      lib/c-ctype.c
      lib/c-strcasecmp.c
      lib/c-strncasecmp.c
      lib/careadlinkat.c
      lib/cloexec.c
      lib/close-stream.c
      lib/closeout.c
      lib/concat-filename.c
      lib/dirname-lgpl.c
      lib/dirname.c
      lib/dup-safer-flag.c
      lib/dup-safer.c
      lib/exitfail.c
      lib/fatal-signal.c
      lib/fcntl.c
      lib/fd-safer-flag.c
      lib/fd-safer.c
      lib/fopen-safer.c
      lib/fstrcmp.c
      lib/get-errno.c
      lib/gethrxtime.c
      lib/getprogname.c
      lib/gettime.c
      lib/gl_array_list.c
      lib/gl_list.c
      lib/gl_xlist.c
      lib/glthread/lock.c
      lib/glthread/threadlib.c
      lib/glthread/tls.c
      lib/hard-locale.c
      lib/hash.c
      lib/isnand.c
      lib/isnanf.c
      lib/isnanl.c
      lib/localcharset.c
      lib/math.c
      lib/mbchar.c
      lib/mbfile.c
      lib/mbrtowc.c
      lib/mbswidth.c
      lib/obstack.c
      lib/path-join.c
      lib/pipe-safer.c
      lib/pipe2-safer.c
      lib/pipe2.c
      lib/printf-args.c
      lib/printf-frexp.c
      lib/printf-frexpl.c
      lib/printf-parse.c
      lib/progname.c
      lib/progreloc.c
      lib/quotearg.c
      lib/relocatable.c
      lib/sig-handler.c
      lib/spawn-pipe.c
      lib/stripslash.c
      lib/timespec.c
      lib/timevar.c
      lib/unistd.c
      lib/uniwidth/width.c
      lib/vasnprintf.c
      lib/wait-process.c
      lib/wctype-h.c
      lib/xalloc-die.c
      lib/xconcat-filename.c
      lib/xhash.c
      lib/xmalloc.c
      lib/xmemdup0.c
      lib/xreadlink.c
      lib/xsize.c
      lib/xstrndup.c
      lib/xtime.c
  DARWIN # cmake-format: sortable
         lib/asprintf.c
         lib/canonicalize-lgpl.c
         lib/error.c
         lib/fpending.c
         lib/fprintf.c
         lib/fseterr.c
         lib/getopt.c
         lib/getopt1.c
         lib/lstat.c
         lib/malloca.c
         lib/obstack_printf.c
         lib/open.c
         lib/perror.c
         lib/printf.c
         lib/readlink.c
         lib/rename.c
         lib/snprintf.c
         lib/sprintf.c
         lib/stat-time.c
         lib/stat.c
         lib/strerror-override.c
         lib/strerror.c
         lib/strerror_r.c
         lib/strverscmp.c
         lib/unlink.c
         lib/vasprintf.c
         lib/vfprintf.c
         lib/vsnprintf.c
         lib/vsprintf.c
         lib/wcwidth.c
         lib/xtime.c)

android_target_include_directories(bison_lib darwin-x86_64 PUBLIC
                                   ${BISON_ROOT}/darwin-lib)
android_target_include_directories(bison_lib linux-x86_64 PUBLIC
                                   ${BISON_ROOT}/linux_glibc-lib)
target_compile_options(bison_lib PRIVATE -Wno-error)

target_include_directories(bison_lib PUBLIC ${BISON_ROOT}/lib)
target_compile_definitions(bison_lib PRIVATE -DEXEEXT="")

android_add_executable(
  TARGET
    bison
    URL
    "https://android.googlesource.com/platform/external/bison/+/refs/heads/master"
  REPO "${BISON_ROOT}"
  NOTICE "REPO/NOTICE"
  LICENSE "GPL3"
  SRC # cmake-format: sortable
      ${BISON_ROOT}/src/AnnotationList.c
      ${BISON_ROOT}/src/assoc.c
      ${BISON_ROOT}/src/closure.c
      ${BISON_ROOT}/src/complain.c
      ${BISON_ROOT}/src/conflicts.c
      ${BISON_ROOT}/src/derives.c
      ${BISON_ROOT}/src/files.c
      ${BISON_ROOT}/src/fixits.c
      ${BISON_ROOT}/src/getargs.c
      ${BISON_ROOT}/src/gram.c
      ${BISON_ROOT}/src/graphviz.c
      ${BISON_ROOT}/src/ielr.c
      ${BISON_ROOT}/src/InadequacyList.c
      ${BISON_ROOT}/src/lalr.c
      ${BISON_ROOT}/src/location.c
      ${BISON_ROOT}/src/lr0.c
      ${BISON_ROOT}/src/main.c
      ${BISON_ROOT}/src/muscle-tab.c
      ${BISON_ROOT}/src/named-ref.c
      ${BISON_ROOT}/src/nullable.c
      ${BISON_ROOT}/src/output.c
      ${BISON_ROOT}/src/parse-gram.c
      ${BISON_ROOT}/src/print-graph.c
      ${BISON_ROOT}/src/print-xml.c
      ${BISON_ROOT}/src/print.c
      ${BISON_ROOT}/src/reader.c
      ${BISON_ROOT}/src/reduce.c
      ${BISON_ROOT}/src/relation.c
      ${BISON_ROOT}/src/Sbitset.c
      ${BISON_ROOT}/src/scan-code-c.c
      ${BISON_ROOT}/src/scan-gram-c.c
      ${BISON_ROOT}/src/scan-skel-c.c
      ${BISON_ROOT}/src/state.c
      ${BISON_ROOT}/src/symlist.c
      ${BISON_ROOT}/src/symtab.c
      ${BISON_ROOT}/src/tables.c
      ${BISON_ROOT}/src/uniqstr.c)
target_include_directories(bison PRIVATE ${BISON_ROOT})

target_link_libraries(bison PRIVATE bison_lib)
target_compile_options(bison PRIVATE -Wno-error)
target_compile_definitions(bison PRIVATE -DINSTALLDIR="${CMAKE_BINARY_DIR}")
