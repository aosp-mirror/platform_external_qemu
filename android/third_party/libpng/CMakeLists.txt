cmake_minimum_required(VERSION 3.0.2)
cmake_policy(VERSION 3.0.2)
project(libpng)
set(PNG_SRC_DIR ${ANDROID_QEMU2_TOP_DIR}/../libpng/)

if(LINUX_AARCH64)
  set(PNG_ARCH_SPECIFIC ${PNG_SRC_DIR}/arm/arm_init.c
                        ${PNG_SRC_DIR}/arm/filter_neon_intrinsics.c)
else()
  set(PNG_ARCH_SPECIFIC ${PNG_SRC_DIR}/intel/intel_init.c
                        ${PNG_SRC_DIR}/intel/filter_sse2_intrinsics.c)
endif()

# TODO(jansene): We can likely turn this into a shared library. Which would save
# around 280kb per executable.
android_add_library(
  TARGET png
  LIBNAME
    libpng
    URL
    "https://android.googlesource.com/platform/external/libpng/+/refs/heads/emu-master-dev"
  REPO "${PNG_SRC_DIR}"
  NOTICE "REPO/LICENSE"
  LICENSE libpng
  SRC # cmake-format: sortable
      ${PNG_ARCH_SPECIFIC}
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
      ${PNG_SRC_DIR}/pngwutil.c)
target_compile_options(
  png PRIVATE -std=gnu89 -Wall -Wno-unused-parameter
              -Wno-tautological-constant-out-of-range-compare)
target_compile_definitions(png PRIVATE -DPNG_INTEL_SSE_OPT=1)
target_include_directories(png PUBLIC ${PNG_SRC_DIR})
target_link_libraries(png PUBLIC zlib)
android_target_link_libraries(png linux-x86_64 PRIVATE m)
android_target_link_libraries(png linux-aarch64 PRIVATE m)
android_target_compile_options(png darwin-x86_64
                               PRIVATE -Wno-unguarded-availability-new)
android_target_compile_options(
  png windows_msvc-x86_64
  PRIVATE -Wno-nonportable-system-include-path
          -Wno-unused-macros
          -Wno-sign-conversion
          -Wno-float-equal
          -Wno-extra-semi-stmt
          -Wno-language-extension-token
          -Wno-reserved-id-macro)

function(png_add_test)
  # We cannot run tests when we are cross compiling.
  if(CROSSCOMPILE)
    return()
  endif()

  set(options)
  set(oneValueArgs NAME COMMAND)
  set(multiValueArgs OPTIONS FILES)
  cmake_parse_arguments(_PAT "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(NOT _PAT_NAME)
    message(FATAL_ERROR "Invalid arguments.  png_add_test requires name.")
  endif()
  if(NOT _PAT_COMMAND)
    message(FATAL_ERROR "Invalid arguments.  png_add_test requires command.")
  endif()

  set(TEST_OPTIONS "${_PAT_OPTIONS}")
  set(TEST_FILES "${_PAT_FILES}")

  add_test(NAME "${_PAT_NAME}" COMMAND "$<TARGET_FILE:${_PAT_COMMAND}>"
                                       ${TEST_OPTIONS} ${TEST_FILES}
           WORKING_DIRECTORY ${PNG_SRC_DIR})
endfunction()

set(pngimage_src)
set(pngfix_src ${PNG_SRC_DIR}/contrib/tools/pngfix.c)
set(png_fix_itxt_src ${PNG_SRC_DIR}/contrib/tools/png-fix-itxt.c)

# Find test PNG files by globbing, but sort lists to ensure consistency between
# different filesystems.
file(GLOB PNGSUITE_PNGS "${PNG_SRC_DIR}/contrib/pngsuite/*.png")
list(SORT PNGSUITE_PNGS)
file(GLOB TEST_PNGS "${PNG_SRC_DIR}/contrib/testpngs/*.png")
list(SORT TEST_PNGS)

set(PNGTEST_PNG "${PNG_SRC_DIR}/pngtest.png")

android_add_executable(TARGET pngtest NODISTRIBUTE SRC # cmake-format: sortable
                                                       ${PNG_SRC_DIR}/pngtest.c)
target_link_libraries(pngtest PRIVATE png)

png_add_test(NAME pngtest COMMAND pngtest FILES "${PNGTEST_PNG}")

android_add_executable(
  TARGET pngvalid NODISTRIBUTE SRC # cmake-format: sortable
                                   ${PNG_SRC_DIR}/contrib/libtests/pngvalid.c)
target_link_libraries(pngvalid PRIVATE png)
target_compile_options(
  pngvalid PRIVATE -Wno-incompatible-pointer-types-discards-qualifiers)
png_add_test(NAME pngvalid-gamma-16-to-8 COMMAND pngvalid
             OPTIONS --gamma-16-to-8)
png_add_test(NAME pngvalid-gamma-alpha-mode COMMAND pngvalid
             OPTIONS --gamma-alpha-mode)
png_add_test(NAME pngvalid-gamma-background COMMAND pngvalid
             OPTIONS --gamma-background)
png_add_test(NAME pngvalid-gamma-expand16-alpha-mode COMMAND pngvalid
             OPTIONS --gamma-alpha-mode --expand16)
png_add_test(NAME pngvalid-gamma-expand16-background COMMAND pngvalid
             OPTIONS --gamma-background --expand16)
png_add_test(NAME pngvalid-gamma-expand16-transform COMMAND pngvalid
             OPTIONS --gamma-transform --expand16)
png_add_test(NAME pngvalid-gamma-sbit COMMAND pngvalid OPTIONS --gamma-sbit)
png_add_test(NAME pngvalid-gamma-threshold COMMAND pngvalid
             OPTIONS --gamma-threshold)
png_add_test(NAME pngvalid-gamma-transform COMMAND pngvalid
             OPTIONS --gamma-transform)
png_add_test(NAME pngvalid-progressive-interlace-standard COMMAND pngvalid
             OPTIONS --standard --progressive-read --interlace)
png_add_test(NAME pngvalid-progressive-size COMMAND pngvalid
             OPTIONS --size --progressive-read)
png_add_test(NAME pngvalid-progressive-standard COMMAND pngvalid
             OPTIONS --standard --progressive-read)
png_add_test(NAME pngvalid-standard COMMAND pngvalid OPTIONS --standard)
png_add_test(NAME pngvalid-transform COMMAND pngvalid OPTIONS --transform)

android_add_executable(
  TARGET pngstest NODISTRIBUTE SRC # cmake-format: sortable
                                   ${PNG_SRC_DIR}/contrib/libtests/pngstest.c)
target_link_libraries(pngstest PRIVATE png)
target_compile_options(
  pngstest PRIVATE "-Wno-tautological-constant-out-of-range-compare")

foreach(gamma_type 1.8 linear none sRGB)
  foreach(alpha_type none alpha)
    set(PNGSTEST_FILES)
    foreach(test_png ${TEST_PNGS})
      string(REGEX MATCH ".*-linear[-.].*" TEST_PNG_LINEAR "${test_png}")
      string(REGEX MATCH ".*-sRGB[-.].*" TEST_PNG_SRGB "${test_png}")
      string(REGEX MATCH ".*-1.8[-.].*" TEST_PNG_G18 "${test_png}")
      string(REGEX MATCH ".*-alpha-.*" TEST_PNG_ALPHA "${test_png}")

      set(TEST_PNG_VALID TRUE)

      if(TEST_PNG_ALPHA)
        if(NOT "${alpha_type}" STREQUAL "alpha")
          set(TEST_PNG_VALID FALSE)
        endif()
      else()
        if("${alpha_type}" STREQUAL "alpha")
          set(TEST_PNG_VALID FALSE)
        endif()
      endif()

      if(TEST_PNG_LINEAR)
        if(NOT "${gamma_type}" STREQUAL "linear")
          set(TEST_PNG_VALID FALSE)
        endif()
      elseif(TEST_PNG_SRGB)
        if(NOT "${gamma_type}" STREQUAL "sRGB")
          set(TEST_PNG_VALID FALSE)
        endif()
      elseif(TEST_PNG_G18)
        if(NOT "${gamma_type}" STREQUAL "1.8")
          set(TEST_PNG_VALID FALSE)
        endif()
      else()
        if(NOT "${gamma_type}" STREQUAL "none")
          set(TEST_PNG_VALID FALSE)
        endif()
      endif()

      if(TEST_PNG_VALID)
        list(APPEND PNGSTEST_FILES "${test_png}")
      endif()
    endforeach()
    # Should already be sorted, but sort anyway to be certain.
    list(SORT PNGSTEST_FILES)
    png_add_test(
      NAME pngstest-${gamma_type}-${alpha_type} COMMAND pngstest
      OPTIONS --tmpfile "${gamma_type}-${alpha_type}-" --log
      FILES ${PNGSTEST_FILES})
  endforeach()
endforeach()

android_add_executable(
  TARGET pngunknown NODISTRIBUTE
  SRC # cmake-format: sortable
      ${PNG_SRC_DIR}/contrib/libtests/pngunknown.c)
target_link_libraries(pngunknown PRIVATE png)

png_add_test(
  NAME pngunknown-discard COMMAND pngunknown OPTIONS --strict default=discard
  FILES "${PNGTEST_PNG}")
png_add_test(NAME pngunknown-IDAT COMMAND pngunknown
             OPTIONS --strict default=discard IDAT=save FILES "${PNGTEST_PNG}")
png_add_test(
  NAME pngunknown-if-safe COMMAND pngunknown OPTIONS --strict default=if-safe
  FILES "${PNGTEST_PNG}")
png_add_test(
  NAME pngunknown-sAPI
  COMMAND pngunknown
  OPTIONS --strict
          bKGD=save
          cHRM=save
          gAMA=save
          all=discard
          iCCP=save
          sBIT=save
          sRGB=save
  FILES "${PNGTEST_PNG}")
png_add_test(
  NAME pngunknown-save COMMAND pngunknown OPTIONS --strict default=save
  FILES "${PNGTEST_PNG}")
png_add_test(
  NAME pngunknown-sTER COMMAND pngunknown OPTIONS --strict sTER=if-safe
  FILES "${PNGTEST_PNG}")
png_add_test(
  NAME pngunknown-vpAg COMMAND pngunknown OPTIONS --strict vpAg=if-safe
  FILES "${PNGTEST_PNG}")

android_add_executable(
  TARGET pngimage NODISTRIBUTE SRC # cmake-format: sortable
                                   ${PNG_SRC_DIR}/contrib/libtests/pngimage.c)
target_link_libraries(pngimage PRIVATE png)
