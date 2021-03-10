# Builds bison that is included with android, currently only supports linux & macos.

# We might be running in cross compile build, so setup the required paths and includes..
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
  LICENSE "GPL3"
  SRC # cmake-format: sortable
      ${BISON_ROOT}/lib/allocator.c
      ${BISON_ROOT}/lib/areadlink.c
      ${BISON_ROOT}/lib/argmatch.c
      ${BISON_ROOT}/lib/asnprintf.c
      ${BISON_ROOT}/lib/asprintf.c
      ${BISON_ROOT}/lib/basename-lgpl.c
      ${BISON_ROOT}/lib/basename.c
      ${BISON_ROOT}/lib/binary-io.c
      ${BISON_ROOT}/lib/bitrotate.c
      ${BISON_ROOT}/lib/bitset.c
      ${BISON_ROOT}/lib/bitset/array.c
      ${BISON_ROOT}/lib/bitset/list.c
      ${BISON_ROOT}/lib/bitset/stats.c
      ${BISON_ROOT}/lib/bitset/table.c
      ${BISON_ROOT}/lib/bitset/vector.c
      ${BISON_ROOT}/lib/bitsetv.c
      ${BISON_ROOT}/lib/c-ctype.c
      ${BISON_ROOT}/lib/c-strcasecmp.c
      ${BISON_ROOT}/lib/c-strncasecmp.c
      ${BISON_ROOT}/lib/canonicalize-lgpl.c
      ${BISON_ROOT}/lib/careadlinkat.c
      ${BISON_ROOT}/lib/cloexec.c
      ${BISON_ROOT}/lib/close-stream.c
      ${BISON_ROOT}/lib/closeout.c
      ${BISON_ROOT}/lib/concat-filename.c
      ${BISON_ROOT}/lib/dirname-lgpl.c
      ${BISON_ROOT}/lib/dirname.c
      ${BISON_ROOT}/lib/dup-safer-flag.c
      ${BISON_ROOT}/lib/dup-safer.c
      ${BISON_ROOT}/lib/error.c
      ${BISON_ROOT}/lib/exitfail.c
      ${BISON_ROOT}/lib/fatal-signal.c
      ${BISON_ROOT}/lib/fd-safer-flag.c
      ${BISON_ROOT}/lib/fd-safer.c
      ${BISON_ROOT}/lib/fopen-safer.c
      ${BISON_ROOT}/lib/fpending.c
      ${BISON_ROOT}/lib/fprintf.c
      ${BISON_ROOT}/lib/fseterr.c
      ${BISON_ROOT}/lib/fstrcmp.c
      ${BISON_ROOT}/lib/get-errno.c
      ${BISON_ROOT}/lib/gethrxtime.c
      ${BISON_ROOT}/lib/getopt.c
      ${BISON_ROOT}/lib/getopt1.c
      ${BISON_ROOT}/lib/getprogname.c
      ${BISON_ROOT}/lib/gettime.c
      ${BISON_ROOT}/lib/gl_array_list.c
      ${BISON_ROOT}/lib/gl_list.c
      ${BISON_ROOT}/lib/gl_xlist.c
      ${BISON_ROOT}/lib/glthread/lock.c
      ${BISON_ROOT}/lib/glthread/threadlib.c
      ${BISON_ROOT}/lib/glthread/tls.c
      ${BISON_ROOT}/lib/hash.c
      ${BISON_ROOT}/lib/isnand.c
      ${BISON_ROOT}/lib/isnanf.c
      ${BISON_ROOT}/lib/isnanl.c
      ${BISON_ROOT}/lib/localcharset.c
      ${BISON_ROOT}/lib/lstat.c
      ${BISON_ROOT}/lib/malloca.c
      ${BISON_ROOT}/lib/math.c
      ${BISON_ROOT}/lib/mbchar.c
      ${BISON_ROOT}/lib/mbfile.c
      ${BISON_ROOT}/lib/mbswidth.c
      ${BISON_ROOT}/lib/obstack.c
      ${BISON_ROOT}/lib/obstack_printf.c
      ${BISON_ROOT}/lib/open.c
      ${BISON_ROOT}/lib/path-join.c
      ${BISON_ROOT}/lib/perror.c
      ${BISON_ROOT}/lib/pipe-safer.c
      ${BISON_ROOT}/lib/pipe2-safer.c
      ${BISON_ROOT}/lib/pipe2.c
      ${BISON_ROOT}/lib/printf-args.c
      ${BISON_ROOT}/lib/printf-frexp.c
      ${BISON_ROOT}/lib/printf-frexpl.c
      ${BISON_ROOT}/lib/printf-parse.c
      ${BISON_ROOT}/lib/printf.c
      ${BISON_ROOT}/lib/progname.c
      ${BISON_ROOT}/lib/progreloc.c
      ${BISON_ROOT}/lib/quotearg.c
      ${BISON_ROOT}/lib/readlink.c
      ${BISON_ROOT}/lib/relocatable.c
      ${BISON_ROOT}/lib/rename.c
      ${BISON_ROOT}/lib/sig-handler.c
      ${BISON_ROOT}/lib/snprintf.c
      ${BISON_ROOT}/lib/spawn-pipe.c
      ${BISON_ROOT}/lib/sprintf.c
      ${BISON_ROOT}/lib/stat-time.c
      ${BISON_ROOT}/lib/stat.c
      ${BISON_ROOT}/lib/strerror-override.c
      ${BISON_ROOT}/lib/strerror.c
      ${BISON_ROOT}/lib/strerror_r.c
      ${BISON_ROOT}/lib/stripslash.c
      ${BISON_ROOT}/lib/strverscmp.c
      ${BISON_ROOT}/lib/timespec.c
      ${BISON_ROOT}/lib/timevar.c
      ${BISON_ROOT}/lib/unistd.c
      ${BISON_ROOT}/lib/uniwidth/width.c
      ${BISON_ROOT}/lib/unlink.c
      ${BISON_ROOT}/lib/vasnprintf.c
      ${BISON_ROOT}/lib/vasprintf.c
      ${BISON_ROOT}/lib/vfprintf.c
      ${BISON_ROOT}/lib/vsnprintf.c
      ${BISON_ROOT}/lib/vsprintf.c
      ${BISON_ROOT}/lib/wait-process.c
      ${BISON_ROOT}/lib/wctype-h.c
      ${BISON_ROOT}/lib/wcwidth.c
      ${BISON_ROOT}/lib/xalloc-die.c
      ${BISON_ROOT}/lib/xconcat-filename.c
      ${BISON_ROOT}/lib/xhash.c
      ${BISON_ROOT}/lib/xmalloc.c
      ${BISON_ROOT}/lib/xmemdup0.c
      ${BISON_ROOT}/lib/xreadlink.c
      ${BISON_ROOT}/lib/xsize.c
      ${BISON_ROOT}/lib/xstrndup.c
      ${BISON_ROOT}/lib/xtime.c)

android_target_include_directories(bison_lib darwin-x86_64 PUBLIC
                                   ${BISON_ROOT}/darwin-lib)
android_target_include_directories(bison_lib linux-x86_64 PUBLIC
                                   ${BISON_ROOT}/linux-lib)

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
target_compile_definitions(bison PRIVATE -DINSTALLDIR="${CMAKE_BINARY_DIR}")
