enable_language(ASM)
android_add_library(
  TARGET crashpad_compat
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${ANDROID_QEMU2_TOP_DIR}/dummy.c
  LINUX # cmake-format: sortable
        linux/sys/mman_memfd_create.cc
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
          win/strings.cc win/time.cc)

target_include_directories(crashpad_compat PUBLIC ..)
android_target_include_directories(crashpad_compat darwin-x86_64 PUBLIC mac non_win)
android_target_include_directories(crashpad_compat darwin-aarch64 PUBLIC mac non_win)
android_target_include_directories(crashpad_compat linux PUBLIC linux non_win)
android_target_include_directories(crashpad_compat windows PUBLIC win)
target_link_libraries(crashpad_compat PUBLIC mini_chromium)
android_target_link_libraries(crashpad_compat linux PUBLIC dl)

android_target_compile_options(crashpad_compat windows PUBLIC /FS /W4 /WX /Zi /bigobj /wd4100 /wd4127 /wd4324 /wd4351 /wd4577 /wd4996 /GL /O2 /Ob2 /Oy- /Zc:inline /d2Zi+ -Wno-error)
android_target_compile_definitions(crashpad_compat windows PUBLIC _UNICODE)
