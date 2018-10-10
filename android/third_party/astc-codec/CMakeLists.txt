cmake_minimum_required(VERSION 3.5)

set(src "../../../../astc-codec/src")

add_library(emulator-astc-codec
        ${src}/decoder/astc_file.cc
        ${src}/decoder/codec.cc
        ${src}/decoder/endpoint_codec.cc
        ${src}/decoder/footprint.cc
        ${src}/decoder/integer_sequence_codec.cc
        ${src}/decoder/intermediate_astc_block.cc
        ${src}/decoder/logical_astc_block.cc
        ${src}/decoder/partition.cc
        ${src}/decoder/physical_astc_block.cc
        ${src}/decoder/quantization.cc
        ${src}/decoder/weight_infill.cc
        )

target_include_directories(emulator-astc-codec PUBLIC ${src}/../include)
target_include_directories(emulator-astc-codec PRIVATE ${src}/..)

if (WIN32 OR ("${ANDROID_TARGET_TAG}" MATCHES "windows.*"))
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # MinGW has some additional warnings that are safe to ignore:
        #  - maybe-uninitialized in intermediate_astc_block.cc, for a std::array
        #    that is safely uninitialzed by a helper function.
        #  - unused-variable in release builds due to asserts. clang doesn't have
        #    this warning enabled, so it's safe to disable here.
        target_compile_definitions(emulator-astc-codec PRIVATE -DLIBYUV_DISABLE_X86)
        target_compile_options(emulator-astc-codec PRIVATE
                -Wno-error=unused-variable
                -Wno-error=maybe-uninitialized)
    endif ()
endif ()

enable_testing()

add_executable(emulator_astc_unittests
        ${src}/base/test/bit_stream_test.cpp
        ${src}/base/test/bottom_n_test.cpp
        ${src}/base/test/math_utils_test.cpp
        ${src}/base/test/optional_test.cpp
        ${src}/base/test/string_utils_test.cpp
        ${src}/base/test/type_traits_test.cpp
        ${src}/base/test/uint128_test.cpp
        ${src}/decoder/test/partition_test.cc
        ${src}/decoder/test/physical_astc_block_test.cc
        ${src}/decoder/test/integer_sequence_codec_test.cc
        ${src}/decoder/test/intermediate_astc_block_test.cc
        ${src}/decoder/test/quantization_test.cc
        ${src}/decoder/test/weight_infill_test.cc
        ${src}/decoder/test/endpoint_codec_test.cc
        ${src}/decoder/test/logical_astc_block_test.cc
        ${src}/decoder/test/codec_test.cc
        ${src}/decoder/test/footprint_test.cc
        )
target_include_directories(emulator_astc_unittests PRIVATE ${src}/..)
target_compile_definitions(emulator_astc_unittests PRIVATE -DGTEST_HAS_RTTI=0)
target_link_libraries(emulator_astc_unittests emulator-astc-codec gmock_main)

IF (UNIX AND NOT APPLE)
    set_target_properties(emulator_astc_unittests PROPERTIES LINK_FLAGS "-Wl,-rpath=$ORIGIN/lib64:$ORIGIN/lib:$ORIGIN/intermediates64")
ENDIF (UNIX AND NOT APPLE)
