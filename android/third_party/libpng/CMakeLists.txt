cmake_minimum_required(VERSION 3.0.2)
cmake_policy(VERSION 3.0.2)
project(libpng)
set(PNG_SRC_DIR ${PNG_SRC_DIR}/${ANDROID_QEMU2_TOP_DIR}/../libpng${PNG_SRC_DIR}/)

set(png_src
    ${PNG_SRC_DIR}/png.c
    ${PNG_SRC_DIR}/pngerror.c
    ${PNG_SRC_DIR}/pngget.c
    ${PNG_SRC_DIR}/pngmem.c
    ${PNG_SRC_DIR}/pngpread.c
    ${PNG_SRC_DIR}/pngread.c
    ${PNG_SRC_DIR}/pngrio.c
    ${PNG_SRC_DIR}/pngrtran.c
    ${PNG_SRC_DIR}/pngrutil.c
    ${PNG_SRC_DIR}/pngset.c
    ${PNG_SRC_DIR}/pngtrans.c
    ${PNG_SRC_DIR}/pngwio.c
    ${PNG_SRC_DIR}/pngwrite.c
    ${PNG_SRC_DIR}/pngwtran.c
    ${PNG_SRC_DIR}/pngwutil.c
    ${PNG_SRC_DIR}/intel/intel_init.c
    ${PNG_SRC_DIR}/intel/filter_sse2_intrinsics.c)

# TODO(jansene): We can likely turn this into a shared library.
# Which would save around 280kb per executable.
android_add_library(png)
target_compile_options(png PRIVATE -std=gnu89 -Wall -Werror -Wno-unused-parameter -Wno-tautological-constant-out-of-range-compare)
target_compile_definitions(png PRIVATE -DPNG_INTEL_SSE_OPT=1)
target_include_directories(png PUBLIC ${PNG_SRC_DIR})
target_link_libraries(png PRIVATE zlib)
android_target_link_libraries(png linux-x86_64 PRIVATE m)
android_target_compile_options(png darwin-x86_64 PRIVATE -Wno-unguarded-availability-new)

set(png_test_src ${PNG_SRC_DIR}/pngtest.c)
android_add_executable(png_test)
target_link_libraries(png_test PRIVATE png zlib)

if(ANDROID_TARGET_TAG STREQUAL ANDROID_HOST_TAG)
  add_test(NAME png_test COMMAND $<TARGET_FILE:png_test> WORKING_DIRECTORY ${PNG_SRC_DIR})
  android_add_default_test_properties(png_test)
endif()
