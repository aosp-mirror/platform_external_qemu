cmake_minimum_required(VERSION 3.5)

set(src "../../../../libyuv/files/source")

add_library(emulator-libyuv
        ${src}/compare.cc
        ${src}/compare_common.cc
        ${src}/compare_gcc.cc
        ${src}/compare_neon64.cc
        ${src}/compare_neon.cc
        ${src}/convert_argb.cc
        ${src}/convert.cc
        ${src}/convert_from_argb.cc
        ${src}/convert_from.cc
        ${src}/convert_jpeg.cc
        ${src}/convert_to_argb.cc
        ${src}/convert_to_i420.cc
        ${src}/cpu_id.cc
        ${src}/mjpeg_decoder.cc
        ${src}/mjpeg_validate.cc
        ${src}/planar_functions.cc
        ${src}/rotate_any.cc
        ${src}/rotate_argb.cc
        ${src}/rotate.cc
        ${src}/rotate_common.cc
        ${src}/rotate_gcc.cc
        ${src}/rotate_dspr2.cc
        ${src}/rotate_neon64.cc
        ${src}/rotate_neon.cc
        ${src}/rotate_win.cc
        ${src}/row_any.cc
        ${src}/row_common.cc
        ${src}/row_gcc.cc
        ${src}/row_dspr2.cc
        ${src}/row_neon64.cc
        ${src}/row_neon.cc
        ${src}/row_win.cc
        ${src}/scale_any.cc
        ${src}/scale_argb.cc
        ${src}/scale.cc
        ${src}/scale_common.cc
        ${src}/scale_gcc.cc
        ${src}/scale_dspr2.cc
        ${src}/scale_neon64.cc
        ${src}/scale_neon.cc
        ${src}/scale_win.cc
        ${src}/video_common.cc
        )

target_include_directories(emulator-libyuv PUBLIC ${src}/../include)
target_compile_options(emulator-libyuv PRIVATE -fno-strict-aliasing)


if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Assembly not worning with mingw, maybe on VS?
        target_compile_definitions(emulator-libyuv PRIVATE -DLIBYUV_DISABLE_X86)
    endif ()
endif ()

