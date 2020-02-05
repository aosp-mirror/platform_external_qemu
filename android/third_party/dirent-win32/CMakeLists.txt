cmake_minimum_required(VERSION 3.5)

# Dirent for windows-msvc.
android_add_interface(
  TARGET dirent-win32
  LIBNAME
    dirent
    URL
    "https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/third_party/dirent-win32"
  REPO "${ANDROID_QEMU2_TOP_DIR}/android/third_party/dirent-win32"
  LICENSE "MIT"
  NOTICE "REPO/NOTICE")
android_target_include_directories(dirent-win32 windows_msvc-x86_64 INTERFACE
                                   include/)
