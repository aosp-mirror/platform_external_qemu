# We might be running in cross compile build, so setup the required paths and includes..
cmake_minimum_required(VERSION 3.5)
project(protobuf)
include(android)
include(prebuilts)

# We might not be using prebuilts
# TODO(jansene): Remove once joshua's zlib changes are in our tree.
if(TARGET zlib)
  add_library(ZLIB::ZLIB alias zlib)
else()
  prebuilt(ZLIB)
endif()

set(protobuf_VERBOSE ON)
set(protobuf_BUILD_SHARED_LIBS_DEFAULT OFF)
set(protobuf_PUBLIC_COMPILE_DEFS -DGOOGLE_PROTOBUF_CMAKE_BUILD -DGOOGLE_PROTOBUF_NO_RTTI)
if(NOT ANDROID_TARGET_TAG STREQUAL "windows-x86_64")
  list(APPEND protobuf_PUBLIC_COMPILE_DEFS -DHAVE_ZLIB)
endif()
if(NOT ANDROID_TARGET_TAG STREQUAL "windows_msvc-x86_64")
  list(APPEND protobuf_PRIVATE_COMPILE_DEFS -DHAVE_PTHREAD)
endif()

set(protobuf_PRIVATE_COMPILE_OPTS -Wno-enum-compare-switch)

set(protobuf_DEBUG_POSTFIX "d" CACHE STRING "Default debug postfix")

set(protobuf_source_dir "${ANDROID_QEMU2_TOP_DIR}/../protobuf")
# Path to main configure script
set(protobuf_CONFIGURE_SCRIPT "${protobuf_source_dir}/configure.ac")

# Parse configure script
set(protobuf_AC_INIT_REGEX "^AC_INIT\\(\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\]\\)$")
file(STRINGS "${protobuf_CONFIGURE_SCRIPT}" protobuf_AC_INIT_LINE LIMIT_COUNT 1 REGEX "^AC_INIT")
# Description
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\1" protobuf_DESCRIPTION "${protobuf_AC_INIT_LINE}")
# Version
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\2" protobuf_VERSION_STRING "${protobuf_AC_INIT_LINE}")
# Contact
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\3" protobuf_CONTACT "${protobuf_AC_INIT_LINE}")
# Parse version tweaks
set(protobuf_VERSION_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\1" protobuf_VERSION_MAJOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\2" protobuf_VERSION_MINOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\3" protobuf_VERSION_PATCH "${protobuf_VERSION_STRING}")
# Package version
set(protobuf_VERSION "${protobuf_VERSION_MAJOR}.${protobuf_VERSION_MINOR}.${protobuf_VERSION_PATCH}")
set(PROTO_DESC "${protobuf_DESCRIPTION}, Version: ${protobuf_VERSION} (${protobuf_VERSION_STRING})")

set(libprotobuf_lite_src
    ${protobuf_source_dir}/src/google/protobuf/arena.cc
    ${protobuf_source_dir}/src/google/protobuf/arenastring.cc
    ${protobuf_source_dir}/src/google/protobuf/extension_set.cc
    ${protobuf_source_dir}/src/google/protobuf/generated_message_util.cc
    ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.cc
    ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.cc
    ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
    ${protobuf_source_dir}/src/google/protobuf/message_lite.cc
    ${protobuf_source_dir}/src/google/protobuf/repeated_field.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/common.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/int128.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/once.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/status.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/statusor.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/time.cc
    ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.cc)


if (ANDROID_TARGET_TAG STREQUAL "windows_msvc-x86_64")
  list(APPEND libprotobuf_lite_src ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc)
endif()
android_add_library(libprotobuf_lite)
if(NOT ANDROID_TARGET_TAG STREQUAL "windows_msvc-x86_64")
  target_link_libraries(libprotobuf_lite PRIVATE Threads::Threads)
endif()
target_compile_definitions(libprotobuf_lite PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
target_compile_options(libprotobuf_lite PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
target_link_libraries(libprotobuf_lite PRIVATE ZLIB::ZLIB)
# Let's not name things liblibxx etc
set_target_properties(libprotobuf_lite PROPERTIES PREFIX "")

target_include_directories(libprotobuf_lite PUBLIC ${protobuf_source_dir}/src)
target_include_directories(libprotobuf_lite PRIVATE ${protobuf_BINARY_DIR} ${protobuf_source_dir}/config)

set(libprotobuf_src
    ${protobuf_source_dir}/src/google/protobuf/any.cc
    ${protobuf_source_dir}/src/google/protobuf/any.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/api.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/compiler/importer.cc
    ${protobuf_source_dir}/src/google/protobuf/compiler/parser.cc
    ${protobuf_source_dir}/src/google/protobuf/descriptor.cc
    ${protobuf_source_dir}/src/google/protobuf/descriptor.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/descriptor_database.cc
    ${protobuf_source_dir}/src/google/protobuf/duration.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/dynamic_message.cc
    ${protobuf_source_dir}/src/google/protobuf/empty.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/extension_set_heavy.cc
    ${protobuf_source_dir}/src/google/protobuf/field_mask.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/generated_message_reflection.cc
    ${protobuf_source_dir}/src/google/protobuf/io/gzip_stream.cc
    ${protobuf_source_dir}/src/google/protobuf/io/printer.cc
    ${protobuf_source_dir}/src/google/protobuf/io/strtod.cc
    ${protobuf_source_dir}/src/google/protobuf/io/tokenizer.cc
    ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.cc
    ${protobuf_source_dir}/src/google/protobuf/map_field.cc
    ${protobuf_source_dir}/src/google/protobuf/message.cc
    ${protobuf_source_dir}/src/google/protobuf/reflection_ops.cc
    ${protobuf_source_dir}/src/google/protobuf/service.cc
    ${protobuf_source_dir}/src/google/protobuf/source_context.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/struct.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/mathlimits.cc
    ${protobuf_source_dir}/src/google/protobuf/stubs/substitute.cc
    ${protobuf_source_dir}/src/google/protobuf/text_format.cc
    ${protobuf_source_dir}/src/google/protobuf/timestamp.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/type.pb.cc
    ${protobuf_source_dir}/src/google/protobuf/unknown_field_set.cc
    ${protobuf_source_dir}/src/google/protobuf/util/field_comparator.cc
    ${protobuf_source_dir}/src/google/protobuf/util/field_mask_util.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/datapiece.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/default_value_objectwriter.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/error_listener.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/field_mask_utility.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/json_escaping.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/json_objectwriter.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/json_stream_parser.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/object_writer.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/proto_writer.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectsource.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectwriter.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info_test_helper.cc
    ${protobuf_source_dir}/src/google/protobuf/util/internal/utility.cc
    ${protobuf_source_dir}/src/google/protobuf/util/json_util.cc
    ${protobuf_source_dir}/src/google/protobuf/util/message_differencer.cc
    ${protobuf_source_dir}/src/google/protobuf/util/time_util.cc
    ${protobuf_source_dir}/src/google/protobuf/util/type_resolver_util.cc
    ${protobuf_source_dir}/src/google/protobuf/wire_format.cc
    ${protobuf_source_dir}/src/google/protobuf/wrappers.pb.cc)

android_add_library(libprotobuf)
target_compile_definitions(libprotobuf PUBLIC ${protobuf_PUBLIC_COMPILE_DEFS})
target_compile_definitions(libprotobuf PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
target_compile_options(libprotobuf PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
target_link_libraries(libprotobuf PRIVATE ZLIB::ZLIB)
target_link_libraries(libprotobuf PUBLIC libprotobuf_lite)
# Let's not name things liblibxx etc
set_target_properties(libprotobuf PROPERTIES PREFIX "")

if(ANDROID_TARGET_TAG STREQUAL ANDROID_HOST_TAG)
  set(libprotoc_src
      ${protobuf_source_dir}/src/google/protobuf/compiler/code_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/command_line_interface.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_service.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_string_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_doc_comment.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_field_base.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_reflection_class.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_context.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_doc_comment.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_extension_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_generator_factory.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_lazy_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_lazy_message_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_map_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_builder.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_builder_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_name_resolver.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_primitive_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_service.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_shared_code_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_string_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_string_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/js/js_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_oneof.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/plugin.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/plugin.pb.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/python/python_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/ruby/ruby_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/subprocess.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/zip_writer.cc)

  android_add_library(libprotoc)
  target_link_libraries(libprotoc libprotobuf)
  target_compile_definitions(libprotoc PUBLIC ${protobuf_PUBLIC_COMPILE_DEFS})
  target_compile_definitions(libprotoc PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
  target_compile_options(libprotoc PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
  target_include_directories(libprotoc
                             PUBLIC ${protobuf_source_dir}/src ${protobuf_BINARY_DIR} ${protobuf_source_dir}/config)
  # Let's not name things liblibxx etc
  set_target_properties(libprotoc PROPERTIES PREFIX "")

  set(protoc_src
      ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc
      ${protobuf_source_dir}/src/google/protobuf/any.cc
      ${protobuf_source_dir}/src/google/protobuf/arena.cc
      ${protobuf_source_dir}/src/google/protobuf/arenastring.cc
      ${protobuf_source_dir}/src/google/protobuf/descriptor.cc
      ${protobuf_source_dir}/src/google/protobuf/descriptor.pb.cc
      ${protobuf_source_dir}/src/google/protobuf/descriptor_database.cc
      ${protobuf_source_dir}/src/google/protobuf/dynamic_message.cc
      ${protobuf_source_dir}/src/google/protobuf/extension_set.cc
      ${protobuf_source_dir}/src/google/protobuf/extension_set_heavy.cc
      ${protobuf_source_dir}/src/google/protobuf/generated_message_reflection.cc
      ${protobuf_source_dir}/src/google/protobuf/generated_message_util.cc
      ${protobuf_source_dir}/src/google/protobuf/map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/message.cc
      ${protobuf_source_dir}/src/google/protobuf/message_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/reflection_ops.cc
      ${protobuf_source_dir}/src/google/protobuf/repeated_field.cc
      ${protobuf_source_dir}/src/google/protobuf/service.cc
      ${protobuf_source_dir}/src/google/protobuf/text_format.cc
      ${protobuf_source_dir}/src/google/protobuf/unknown_field_set.cc
      ${protobuf_source_dir}/src/google/protobuf/wire_format.cc
      ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/code_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/command_line_interface.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_service.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_string_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_doc_comment.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_field_base.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_reflection_class.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/importer.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_context.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_doc_comment.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_enum_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_extension_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_generator_factory.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_lazy_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_lazy_message_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_map_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_builder.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_builder_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_message_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_name_resolver.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_primitive_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_service.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_shared_code_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_string_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_string_field_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javamicro/javamicro_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/javanano/javanano_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/js/js_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_enum.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_enum_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_extension.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_file.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_helpers.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_map_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_message.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_message_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_oneof.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/parser.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/plugin.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/plugin.pb.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/python/python_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/ruby/ruby_generator.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/subprocess.cc
      ${protobuf_source_dir}/src/google/protobuf/compiler/zip_writer.cc
      ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.cc
      ${protobuf_source_dir}/src/google/protobuf/io/gzip_stream.cc
      ${protobuf_source_dir}/src/google/protobuf/io/printer.cc
      ${protobuf_source_dir}/src/google/protobuf/io/strtod.cc
      ${protobuf_source_dir}/src/google/protobuf/io/tokenizer.cc
      ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.cc
      ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.cc
      ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
      # ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc Excluded due to odr violations.
      ${protobuf_source_dir}/src/google/protobuf/stubs/common.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/int128.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/once.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/status.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/substitute.cc
      ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.cc)

  if (ANDROID_TARGET_TAG STREQUAL "windows_msvc-x86_64")
    list(APPEND protoc_src ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc)
  endif()
  android_add_executable(protoc)
  add_executable(protobuf::protoc ALIAS protoc)
  target_link_libraries(protoc PRIVATE libprotobuf libprotoc)
  target_compile_definitions(protoc PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
  target_compile_options(protoc PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
  set(PROTOBUF_PROTOC_EXECUTABLE "$<TARGET_FILE:protoc>" CACHE PATH "Protocol buffer executable" FORCE)
else()
  message(STATUS "Including protoc as cross compiled external project")
  include(ExternalProject)
  externalproject_add(
    protoc_cross
    PREFIX
    protoc_cross
    DOWNLOAD_COMMAND
    ""
    SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}"
    CMAKE_ARGS
    "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-${ANDROID_HOST_TAG}.cmake"
    BUILD_BYPRODUCTS
    ${CMAKE_CURRENT_BINARY_DIR}/protoc_cross/src/protoc_cross-build/protoc
    INSTALL_COMMAND
    "")
  # We need this property to survive outside of our project.
  set(PROTOBUF_PROTOC_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/protoc_cross/src/protoc_cross-build/protoc
      CACHE PATH "Protocol buffer executable"
      FORCE)
endif()

set(PROTOBUF_LIBRARY "${PROTO_DESC}")
set(PROTOBUF_INCLUDE_DIR "${protobuf_source_dir}/src")

# Later versions of cmake want this to be lower case..
set(Protobuf_INCLUDE_DIR "${PROTOBUF_INCLUDE_DIR}")
set(Protobuf_LIBRARY "${PROTOBUF_LIBRARY}")
set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")
find_package(Protobuf REQUIRED)
if(NOT "${_PROTOBUF_PROTOC_EXECUTABLE_VERSION}" VERSION_EQUAL "${Protobuf_VERSION}")
  message(
    STATUS
      "It's safe to ignore version warnings, it's a result of compiling protoc in the tree with CMake version > 3.5.")
endif()
