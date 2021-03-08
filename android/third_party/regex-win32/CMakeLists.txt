android_add_library(
  TARGET emulator-regex-win32
  SRC # cmake-format: sortable
      compat/reallocarr.c compat/strlcpy.c src/regcomp.c src/regerror.c
      src/regexec.c src/regfree.c
  LIBNAME
    regex
    URL
    "https://android.googlesource.com/platform/external/qemu.git/+/refs/heads/emu-master-dev/android/third_party/regex-win32/"
  LICENSE "BSD-4-Clause"
  NOTICE "REPO/NOTICE"
  REPO "${ANDROID_QEMU2_TOP_DIR}/android/third_party/regex-win32")

target_include_directories(emulator-regex-win32 PUBLIC include)
target_include_directories(emulator-regex-win32 PRIVATE compat)
