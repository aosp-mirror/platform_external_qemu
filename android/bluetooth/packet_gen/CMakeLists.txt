# We might be running in cross compile build, so setup the required paths and includes..
cmake_minimum_required(VERSION 3.5)
project(bluetooth_packetgen)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
endif()

include(android)
include(prebuilts)
set(AOSP_ROOT ${ANDROID_QEMU2_TOP_DIR}/../..)
set(BT_ROOT ${AOSP_ROOT}/packages/modules/Bluetooth/system)
set(PACKET_GEN_ROOT ${BT_ROOT}/gd/packet/parser/)
set(CMAKE_CXX_STANDARD 17)

if(OPTIONS_BUILD_PACKET_GEN)
  android_yacc_compile(
    GENERATED blueotooth_packetgen_yacc SRC ${PACKET_GEN_ROOT}/language_y.yy
    SOURCE_DIR ${PACKET_GEN_ROOT}
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yacc)
  android_flex_compile(
    GENERATED blueotooth_packetgen_flex SRC ${PACKET_GEN_ROOT}/language_l.ll
    SOURCE_DIR ${PACKET_GEN_ROOT}
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/flex)
  message(
    STATUS
      "Generating ${blueotooth_packetgen_yacc} and ${blueotooth_packetgen_flex}, these are places in the SOURCE TREE!"
  )
  android_add_library(
    TARGET bluetooth_packetgen_parser NODISTRIBUTE
    SRC ${blueotooth_packetgen_flex} ${blueotooth_packetgen_yacc})
  target_include_directories(
    bluetooth_packetgen_parser PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/yacc
                                      ${PACKET_GEN_ROOT})
else()
  if(NOT WIN32)
    # Sha validation only works on posix for some reason.
    android_validate_sha(
      FILE ${PACKET_GEN_ROOT}/language_y.yy
      EXPECTED babcc990e9ffbce28821a0451e98370e219ad0fb761047720d33fba625e12135
      ERROR
        "Please regenerate the precompiled yacc files, rebuild with --cmake_option OPTIONS_BUILD_PACKET_GEN=TRUE, update the SHA, and commit the created files."
    )
    android_validate_sha(
      FILE ${PACKET_GEN_ROOT}/language_l.ll
      EXPECTED 9dd4fe0748c25024db18e82ca1995f5d745ce227e6ca763d6c1e73e6001a0075
      ERROR
        "Please regenerate the precompiled flex files, rebuild with --cmake_option OPTIONS_BUILD_PACKET_GEN=TRUE, update the SHA, and commit the created files."
    )
  endif()
  android_add_library(TARGET bluetooth_packetgen_parser NODISTRIBUTE
                      SRC flex/language_l.cpp yacc/language_y.cpp)
  target_include_directories(bluetooth_packetgen_parser
                             PUBLIC yacc ${PACKET_GEN_ROOT})

  target_compile_options(bluetooth_packetgen_parser
                         PRIVATE -Wno-inconsistent-missing-override)
  android_target_compile_options(bluetooth_packetgen_parser darwin-x86_64
                                 PRIVATE "-mmacosx-version-min=10.15")
endif()

android_add_executable(
  TARGET bluetooth_packetgen
  NODISTRIBUTE
  LICENSE Apache-2.0 SOURCE_DIR ${PACKET_GEN_ROOT}
  SRC # cmake-format: sortable
      ${blueotooth_packetgen_flex}
      ${blueotooth_packetgen_yacc}
      checksum_def.cc
      custom_field_def.cc
      enum_def.cc
      enum_gen.cc
      fields/array_field.cc
      fields/body_field.cc
      fields/checksum_field.cc
      fields/checksum_start_field.cc
      fields/count_field.cc
      fields/custom_field.cc
      fields/custom_field_fixed_size.cc
      fields/enum_field.cc
      fields/fixed_enum_field.cc
      fields/fixed_field.cc
      fields/fixed_scalar_field.cc
      fields/group_field.cc
      fields/packet_field.cc
      fields/padding_field.cc
      fields/payload_field.cc
      fields/reserved_field.cc
      fields/scalar_field.cc
      fields/size_field.cc
      fields/struct_field.cc
      fields/variable_length_struct_field.cc
      fields/vector_field.cc
      gen_cpp.cc
      gen_rust.cc
      main.cc
      packet_def.cc
      parent_def.cc
      struct_def.cc
      struct_parser_generator.cc)

target_include_directories(bluetooth_packetgen PRIVATE ${PACKET_GEN_ROOT})
target_link_libraries(bluetooth_packetgen PRIVATE bluetooth_packetgen_parser)
target_compile_options(bluetooth_packetgen
                       PRIVATE "-std=c++17" -Wno-inconsistent-missing-override)

# The packet generator uses std::filesystem which is only available in Sdk10.15.
# Note that the packet_generator is only used on the host, and is not shipped.
android_target_compile_options(bluetooth_packetgen darwin-x86_64
                               PRIVATE "-mmacosx-version-min=10.15")
