cmake_minimum_required(VERSION 3.5)

set(emulator-libjpeg_src)
android_add_library(
  TARGET emulator-libjpeg
  LIBNAME
    libjpeg
    URL
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev/android/third_party/jpeg-6b/"
  REPO "${ANDROID_QEMU2_TOP_DIR}/android/third_party/jpeg-6b/"
  NOTICE "REPO/NOTICE"
  LICENSE IJG
  SRC # cmake-format: sortable
      jcapimin.c
      jcapistd.c
      jccoefct.c
      jccolor.c
      jcdctmgr.c
      jchuff.c
      jcinit.c
      jcmainct.c
      jcmarker.c
      jcmaster.c
      jcomapi.c
      jcparam.c
      jcphuff.c
      jcprepct.c
      jcsample.c
      jctrans.c
      jdapimin.c
      jdapistd.c
      jdatadst.c
      jdatasrc.c
      jdcoefct.c
      jdcolor.c
      jddctmgr.c
      jdhuff.c
      jdinput.c
      jdmainct.c
      jdmarker.c
      jdmaster.c
      jdmerge.c
      jdphuff.c
      jdpostct.c
      jdsample.c
      jdtrans.c
      jerror.c
      jfdctflt.c
      jfdctfst.c
      jfdctint.c
      jidctflt.c
      jidctfst.c
      jidctint.c
      jidctintelsse.c
      jidctred.c
      jmem-android.c
      jmemmgr.c
      jquant1.c
      jquant2.c
      jutils.c)
target_include_directories(emulator-libjpeg PUBLIC .)
if(NOT LINUX_AARCH64)
  target_compile_options(emulator-libjpeg PRIVATE -O3 -fstrict-aliasing -msse2
                                                  -Wno-all)
  target_compile_definitions(
    emulator-libjpeg PRIVATE -DAVOID_TABLES -DANDROID_INTELSSE2_IDCT
                             -DANDROID_TILE_BASED_DECODE)
else()
  target_compile_options(emulator-libjpeg PRIVATE -O3 -fstrict-aliasing
                                                  -Wno-all)
  target_compile_definitions(emulator-libjpeg
                             PRIVATE -DAVOID_TABLES -DANDROID_TILE_BASED_DECODE)
endif()
target_link_libraries(emulator-libjpeg PRIVATE android-emu-base)
