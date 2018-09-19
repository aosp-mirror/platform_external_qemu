cmake_minimum_required(VERSION 3.5)

add_library(emulator-libjpeg
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
    jmemmgr.c
    jmem-android.c
    jquant1.c
    jquant2.c
    jutils.c
)
target_include_directories(emulator-libjpeg PRIVATE .)
target_compile_options(emulator-libjpeg PRIVATE
    -O3
    -fstrict-aliasing
    -msse2
    -Wno-all)
target_compile_definitions(emulator-libjpeg PRIVATE
    -DAVOID_TABLES
    -DANDROID_INTELSSE2_IDCT
    -DANDROID_TILE_BASED_DECODE)



