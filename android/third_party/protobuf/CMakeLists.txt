# We might be running in cross compile build, so setup the required paths and includes..
cmake_minimum_required(VERSION 3.5)
project(protobuf)
include(android)
include(prebuilts)

if(NOT TARGET zlib)
  # Cross build needs zlib for protoc.
  add_subdirectory(../zlib zlib)
endif()

set(protobuf_PUBLIC_COMPILE_DEFS -DGOOGLE_PROTOBUF_CMAKE_BUILD)
list(APPEND protobuf_PUBLIC_COMPILE_DEFS -DHAVE_ZLIB)
if(NOT WINDOWS_MSVC_X86_64)
  list(APPEND protobuf_PRIVATE_COMPILE_DEFS -DHAVE_PTHREAD)
endif()

# Disable sanitizers, as we will be running protoc, which has some ODR
# violations.
set(protobuf_PRIVATE_COMPILE_OPTS
    -Wno-enum-compare-switch -Wno-inconsistent-missing-override
    -Wno-unused-private-field -DANDROID_EMULATOR=1)

set(protobuf_DEBUG_POSTFIX "d" CACHE STRING "Default debug postfix")

set(protobuf_source_dir "${ANDROID_QEMU2_TOP_DIR}/../protobuf")
# Path to main configure script
set(protobuf_CONFIGURE_SCRIPT "${protobuf_source_dir}/configure.ac")

# Parse configure script
set(protobuf_AC_INIT_REGEX
    "^AC_INIT\\(\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\]\\)$")
file(STRINGS "${protobuf_CONFIGURE_SCRIPT}" protobuf_AC_INIT_LINE LIMIT_COUNT 1
     REGEX "^AC_INIT")
# Description
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\1" protobuf_DESCRIPTION
                     "${protobuf_AC_INIT_LINE}")
# Version
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\2" protobuf_VERSION_STRING
                     "${protobuf_AC_INIT_LINE}")
# Contact
string(REGEX REPLACE "${protobuf_AC_INIT_REGEX}" "\\3" protobuf_CONTACT
                     "${protobuf_AC_INIT_LINE}")
# Parse version tweaks
set(protobuf_VERSION_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\1" protobuf_VERSION_MAJOR
                     "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\2" protobuf_VERSION_MINOR
                     "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "${protobuf_VERSION_REGEX}" "\\3" protobuf_VERSION_PATCH
                     "${protobuf_VERSION_STRING}")
# Package version
set(protobuf_VERSION
    "${protobuf_VERSION_MAJOR}.${protobuf_VERSION_MINOR}.${protobuf_VERSION_PATCH}"
)
set(PROTO_DESC
    "${protobuf_DESCRIPTION}, Version: ${protobuf_VERSION} (${protobuf_VERSION_STRING})"
)

android_add_library(
  TARGET libprotobuf_lite
  LIBNAME
    protobuf
    URL
    "https://android.googlesource.com/platform/external/protobuf/+/refs/heads/emu-master-dev"
  REPO "${protobuf_source_dir}"
  LICENSE "BSD-3-Clause"
  NOTICE "REPO/NOTICE"
  SOURCE_DIR ${protobuf_source_dir}
  SRC # AUTOGEN(protobuf_lite_srcs)
      src/google/protobuf/any_lite.cc
      src/google/protobuf/arena.cc
      src/google/protobuf/arenastring.cc
      src/google/protobuf/extension_set.cc
      src/google/protobuf/generated_enum_util.cc
      src/google/protobuf/generated_message_table_driven_lite.cc
      src/google/protobuf/generated_message_tctable_lite.cc
      src/google/protobuf/generated_message_util.cc
      src/google/protobuf/implicit_weak_message.cc
      src/google/protobuf/inlined_string_field.cc
      src/google/protobuf/io/coded_stream.cc
      src/google/protobuf/io/io_win32.cc
      src/google/protobuf/io/strtod.cc
      src/google/protobuf/io/zero_copy_stream.cc
      src/google/protobuf/io/zero_copy_stream_impl.cc
      src/google/protobuf/io/zero_copy_stream_impl_lite.cc
      src/google/protobuf/map.cc
      src/google/protobuf/message_lite.cc
      src/google/protobuf/parse_context.cc
      src/google/protobuf/repeated_field.cc
      src/google/protobuf/stubs/bytestream.cc
      src/google/protobuf/stubs/common.cc
      src/google/protobuf/stubs/int128.cc
      src/google/protobuf/stubs/status.cc
      src/google/protobuf/stubs/statusor.cc
      src/google/protobuf/stubs/stringpiece.cc
      src/google/protobuf/stubs/stringprintf.cc
      src/google/protobuf/stubs/structurally_valid.cc
      src/google/protobuf/stubs/strutil.cc
      src/google/protobuf/stubs/time.cc
      src/google/protobuf/wire_format_lite.cc)

if(NOT WINDOWS_MSVC_X86_64)
  target_link_libraries(libprotobuf_lite PRIVATE Threads::Threads)
endif()
android_target_link_libraries(libprotobuf_lite linux-x86_64 PRIVATE m)
android_target_compile_options(libprotobuf_lite windows_msvc-x86_64
                               PUBLIC -Wno-microsoft-unqualified-friend)
target_compile_definitions(libprotobuf_lite
                           PUBLIC ${protobuf_PUBLIC_COMPILE_DEFS})
target_compile_definitions(libprotobuf_lite
                           PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
target_compile_options(libprotobuf_lite
                       PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS} -Wno-invalid-noreturn)
target_link_libraries(libprotobuf_lite PRIVATE zlib)
# Let's not name things liblibxx etc
set_target_properties(libprotobuf_lite PROPERTIES PREFIX "")

target_include_directories(libprotobuf_lite PUBLIC ${protobuf_source_dir}/src)
target_include_directories(
  libprotobuf_lite PRIVATE ${protobuf_BINARY_DIR} ${protobuf_source_dir}/config)

android_add_library(
  TARGET
    libprotobuf
    URL
    "https://android.googlesource.com/platform/external/protobuf/+/refs/heads/emu-master-dev"
  REPO "${protobuf_source_dir}"
  LICENSE "BSD-3-Clause"
  NOTICE "REPO/NOTICE"
  SOURCE_DIR ${protobuf_source_dir}
  SRC # AUTOGEN(protobuf_srcs)
      src/google/protobuf/any.cc
      src/google/protobuf/any.pb.cc
      src/google/protobuf/api.pb.cc
      src/google/protobuf/compiler/importer.cc
      src/google/protobuf/compiler/parser.cc
      src/google/protobuf/descriptor.cc
      src/google/protobuf/descriptor.pb.cc
      src/google/protobuf/descriptor_database.cc
      src/google/protobuf/duration.pb.cc
      src/google/protobuf/dynamic_message.cc
      src/google/protobuf/empty.pb.cc
      src/google/protobuf/extension_set_heavy.cc
      src/google/protobuf/field_mask.pb.cc
      src/google/protobuf/generated_message_bases.cc
      src/google/protobuf/generated_message_reflection.cc
      src/google/protobuf/generated_message_table_driven.cc
      src/google/protobuf/generated_message_tctable_full.cc
      src/google/protobuf/io/gzip_stream.cc
      src/google/protobuf/io/printer.cc
      src/google/protobuf/io/tokenizer.cc
      src/google/protobuf/map_field.cc
      src/google/protobuf/message.cc
      src/google/protobuf/reflection_ops.cc
      src/google/protobuf/service.cc
      src/google/protobuf/source_context.pb.cc
      src/google/protobuf/struct.pb.cc
      src/google/protobuf/stubs/substitute.cc
      src/google/protobuf/text_format.cc
      src/google/protobuf/timestamp.pb.cc
      src/google/protobuf/type.pb.cc
      src/google/protobuf/unknown_field_set.cc
      src/google/protobuf/util/delimited_message_util.cc
      src/google/protobuf/util/field_comparator.cc
      src/google/protobuf/util/field_mask_util.cc
      src/google/protobuf/util/internal/datapiece.cc
      src/google/protobuf/util/internal/default_value_objectwriter.cc
      src/google/protobuf/util/internal/error_listener.cc
      src/google/protobuf/util/internal/field_mask_utility.cc
      src/google/protobuf/util/internal/json_escaping.cc
      src/google/protobuf/util/internal/json_objectwriter.cc
      src/google/protobuf/util/internal/json_stream_parser.cc
      src/google/protobuf/util/internal/object_writer.cc
      src/google/protobuf/util/internal/proto_writer.cc
      src/google/protobuf/util/internal/protostream_objectsource.cc
      src/google/protobuf/util/internal/protostream_objectwriter.cc
      src/google/protobuf/util/internal/type_info.cc
      src/google/protobuf/util/internal/utility.cc
      src/google/protobuf/util/json_util.cc
      src/google/protobuf/util/message_differencer.cc
      src/google/protobuf/util/time_util.cc
      src/google/protobuf/util/type_resolver_util.cc
      src/google/protobuf/wire_format.cc
      src/google/protobuf/wrappers.pb.cc)
target_compile_definitions(libprotobuf PUBLIC ${protobuf_PUBLIC_COMPILE_DEFS})
target_compile_definitions(libprotobuf PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
target_compile_options(libprotobuf PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
android_target_compile_options(libprotobuf windows_msvc-x86_64
                               PUBLIC -Wno-microsoft-unqualified-friend)
target_link_libraries(libprotobuf PUBLIC zlib libprotobuf_lite)
# Let's not name things liblibxx etc
set_target_properties(libprotobuf PROPERTIES PREFIX "")

if(NOT CROSSCOMPILE)
  android_add_library(
    TARGET libprotoc
    LIBNAME
      protobuf
      URL
      "https://android.googlesource.com/platform/external/protobuf/+/refs/heads/emu-master-dev"
    REPO "${protobuf_source_dir}"
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/NOTICE"
    SOURCE_DIR ${protobuf_source_dir}
    SRC # AUTOGEN(protoc_lib_srcs)
        src/google/protobuf/compiler/code_generator.cc
        src/google/protobuf/compiler/command_line_interface.cc
        src/google/protobuf/compiler/cpp/cpp_enum.cc
        src/google/protobuf/compiler/cpp/cpp_enum_field.cc
        src/google/protobuf/compiler/cpp/cpp_extension.cc
        src/google/protobuf/compiler/cpp/cpp_field.cc
        src/google/protobuf/compiler/cpp/cpp_file.cc
        src/google/protobuf/compiler/cpp/cpp_generator.cc
        src/google/protobuf/compiler/cpp/cpp_helpers.cc
        src/google/protobuf/compiler/cpp/cpp_map_field.cc
        src/google/protobuf/compiler/cpp/cpp_message.cc
        src/google/protobuf/compiler/cpp/cpp_message_field.cc
        src/google/protobuf/compiler/cpp/cpp_padding_optimizer.cc
        src/google/protobuf/compiler/cpp/cpp_parse_function_generator.cc
        src/google/protobuf/compiler/cpp/cpp_primitive_field.cc
        src/google/protobuf/compiler/cpp/cpp_service.cc
        src/google/protobuf/compiler/cpp/cpp_string_field.cc
        src/google/protobuf/compiler/plugin.cc
        src/google/protobuf/compiler/plugin.pb.cc
        src/google/protobuf/compiler/subprocess.cc
        src/google/protobuf/compiler/zip_writer.cc)
  target_link_libraries(libprotoc PRIVATE libprotobuf zlib)
  target_compile_definitions(libprotoc PUBLIC ${protobuf_PUBLIC_COMPILE_DEFS})
  target_compile_definitions(libprotoc PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
  target_compile_options(libprotoc PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
  # Let's not name things liblibxx etc
  target_include_directories(
    libprotoc PUBLIC ${protobuf_source_dir}/src ${protobuf_BINARY_DIR}
                     ${protobuf_source_dir}/config ${protobuf_source_dir})
  set_target_properties(libprotoc PROPERTIES PREFIX "")
  android_add_executable(
    TARGET protoc
    LIBNAME
      protobuf
      URL
      "https://android.googlesource.com/platform/external/protobuf/+/refs/heads/emu-master-dev"
    REPO "${protobuf_source_dir}"
    LICENSE "BSD-3-Clause"
    NOTICE "REPO/NOTICE"
    SRC ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc)
  add_executable(protobuf::protoc ALIAS protoc)
  target_link_libraries(protoc PRIVATE libprotobuf libprotoc zlib)
  target_compile_definitions(protoc PRIVATE ${protobuf_PRIVATE_COMPILE_DEFS})
  target_compile_options(protoc PRIVATE ${protobuf_PRIVATE_COMPILE_OPTS})
  set(PROTOBUF_PROTOC_EXECUTABLE "$<TARGET_FILE:protoc>"
      CACHE PATH "Protocol buffer executable" FORCE)
else()
  android_compile_for_host(protoc ${CMAKE_CURRENT_LIST_DIR} PROTOBUF_EXE)
  set(PROTOBUF_PROTOC_EXECUTABLE ${PROTOBUF_EXE}
      CACHE PATH "Protocol buffer executable" FORCE)
  add_executable(protobuf::protoc IMPORTED GLOBAL)
  set_target_properties(
    protobuf::protoc PROPERTIES IMPORTED_LOCATION
                                "${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

set(PROTOBUF_LIBRARY "${PROTO_DESC}")
set(PROTOBUF_INCLUDE_DIR "${protobuf_source_dir}/src")

# Later versions of cmake want this to be lower case..
set(Protobuf_INCLUDE_DIR "${PROTOBUF_INCLUDE_DIR}")
set(Protobuf_LIBRARY "${PROTOBUF_LIBRARY}")
set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")
find_package(Protobuf REQUIRED)
if(NOT "${_PROTOBUF_PROTOC_EXECUTABLE_VERSION}" VERSION_EQUAL
   "${Protobuf_VERSION}")
  message(
    STATUS
      "It's safe to ignore version warnings, it's a result of compiling protoc in the tree with CMake version > 3.5."
  )
endif()

# Unit tests, only active when we are not cross compiling
if(WINDOWS_MSVC_X86_64)
  message(STATUS "Disabling protobuf tests on MSVC! b/149265836")
elseif(NOT ANDROID_HOST_TAG STREQUAL ANDROID_TARGET_TAG OR NOT TARGET
                                                           gmock_main)
  message(STATUS "Disabling protobuf tests when cross compiling")
else()
  set(lite_test_protos
      # AUTOGEN(lite_test_protos)
      google/protobuf/map_lite_unittest.proto
      google/protobuf/unittest_import_lite.proto
      google/protobuf/unittest_import_public_lite.proto
      google/protobuf/unittest_lite.proto)

  set(tests_protos
      # AUTOGEN(test_protos)
      google/protobuf/any_test.proto
      google/protobuf/compiler/cpp/cpp_test_bad_identifiers.proto
      google/protobuf/compiler/cpp/cpp_test_large_enum_value.proto
      google/protobuf/map_proto2_unittest.proto
      google/protobuf/map_unittest.proto
      google/protobuf/unittest.proto
      google/protobuf/unittest_arena.proto
      google/protobuf/unittest_custom_options.proto
      google/protobuf/unittest_drop_unknown_fields.proto
      google/protobuf/unittest_embed_optimize_for.proto
      google/protobuf/unittest_empty.proto
      google/protobuf/unittest_enormous_descriptor.proto
      google/protobuf/unittest_import.proto
      google/protobuf/unittest_import_public.proto
      google/protobuf/unittest_lazy_dependencies.proto
      google/protobuf/unittest_lazy_dependencies_custom_option.proto
      google/protobuf/unittest_lazy_dependencies_enum.proto
      google/protobuf/unittest_lite_imports_nonlite.proto
      google/protobuf/unittest_mset.proto
      google/protobuf/unittest_mset_wire_format.proto
      google/protobuf/unittest_no_field_presence.proto
      google/protobuf/unittest_no_generic_services.proto
      google/protobuf/unittest_optimize_for.proto
      google/protobuf/unittest_preserve_unknown_enum.proto
      google/protobuf/unittest_preserve_unknown_enum2.proto
      google/protobuf/unittest_proto3.proto
      google/protobuf/unittest_proto3_arena.proto
      google/protobuf/unittest_proto3_arena_lite.proto
      google/protobuf/unittest_proto3_lite.proto
      google/protobuf/unittest_proto3_optional.proto
      google/protobuf/unittest_well_known_types.proto
      google/protobuf/util/internal/testdata/anys.proto
      google/protobuf/util/internal/testdata/books.proto
      google/protobuf/util/internal/testdata/default_value.proto
      google/protobuf/util/internal/testdata/default_value_test.proto
      google/protobuf/util/internal/testdata/field_mask.proto
      google/protobuf/util/internal/testdata/maps.proto
      google/protobuf/util/internal/testdata/oneofs.proto
      google/protobuf/util/internal/testdata/proto3.proto
      google/protobuf/util/internal/testdata/struct.proto
      google/protobuf/util/internal/testdata/timestamp_duration.proto
      google/protobuf/util/internal/testdata/wrappers.proto
      google/protobuf/util/json_format.proto
      google/protobuf/util/json_format_proto3.proto
      google/protobuf/util/message_differencer_unittest.proto)

  macro(compile_proto_file filename)
    get_filename_component(dirname ${filename} PATH)
    get_filename_component(basename ${filename} NAME_WE)
    add_custom_command(
      OUTPUT ${protobuf_source_dir}/src/${dirname}/${basename}.pb.cc
      DEPENDS protoc ${protobuf_source_dir}/src/${dirname}/${basename}.proto
      COMMAND
        protoc ${protobuf_source_dir}/src/${dirname}/${basename}.proto
        --proto_path=${protobuf_source_dir}/src
        --cpp_out=${protobuf_source_dir}/src)
  endmacro(compile_proto_file)

  set(lite_test_proto_files)
  foreach(proto_file ${lite_test_protos})
    compile_proto_file(${proto_file})
    string(REPLACE .proto .pb.cc pb_file ${proto_file})
    set(lite_test_proto_files ${lite_test_proto_files}
                              ${protobuf_source_dir}/src/${pb_file})
  endforeach(proto_file)

  set(tests_proto_files)
  foreach(proto_file ${tests_protos})
    compile_proto_file(${proto_file})
    string(REPLACE .proto .pb.cc pb_file ${proto_file})
    set(tests_proto_files ${tests_proto_files}
                          ${protobuf_source_dir}/src/${pb_file})
  endforeach(proto_file)

  android_add_executable(
    TARGET protobuf_test_plugin
    NODISTRIBUTE SOURCE_DIR ${protobuf_source_dir}
    SRC # AUTOGEN(test_plugin_srcs)
        src/google/protobuf/compiler/mock_code_generator.cc
        src/google/protobuf/compiler/test_plugin.cc
        src/google/protobuf/testing/file.cc)
  target_link_libraries(protobuf_test_plugin PRIVATE libprotoc libprotobuf
                                                     gmock_main gtest)

  android_add_test(
    TARGET protobuf_tests
    SOURCE_DIR ${protobuf_source_dir}
    SRC # AUTOGEN(common_test_srcs)
        ${lite_test_proto_files}
        # AUTOGEN(test_srcs)
        ${tests_proto_files}
        src/google/protobuf/any_test.cc
        src/google/protobuf/arena_test_util.cc
        src/google/protobuf/arena_unittest.cc
        src/google/protobuf/arenastring_unittest.cc
        src/google/protobuf/compiler/annotation_test_util.cc
        src/google/protobuf/compiler/command_line_interface_unittest.cc
        src/google/protobuf/compiler/cpp/cpp_bootstrap_unittest.cc
        src/google/protobuf/compiler/cpp/cpp_move_unittest.cc
        src/google/protobuf/compiler/cpp/cpp_plugin_unittest.cc
        src/google/protobuf/compiler/cpp/cpp_unittest.cc
        src/google/protobuf/compiler/cpp/cpp_unittest.inc
        src/google/protobuf/compiler/cpp/metadata_test.cc
        # src/google/protobuf/compiler/csharp/csharp_bootstrap_unittest.cc
        # src/google/protobuf/compiler/csharp/csharp_generator_unittest.cc
        src/google/protobuf/compiler/importer_unittest.cc
        # src/google/protobuf/compiler/java/java_doc_comment_unittest.cc
        # src/google/protobuf/compiler/java/java_plugin_unittest.cc
        src/google/protobuf/compiler/mock_code_generator.cc
        # src/google/protobuf/compiler/objectivec/objectivec_helpers_unittest.cc
        src/google/protobuf/compiler/parser_unittest.cc
        # src/google/protobuf/compiler/python/python_plugin_unittest.cc
        # src/google/protobuf/compiler/ruby/ruby_generator_unittest.cc
        src/google/protobuf/descriptor_database_unittest.cc
        src/google/protobuf/descriptor_unittest.cc
        src/google/protobuf/drop_unknown_fields_test.cc
        src/google/protobuf/dynamic_message_unittest.cc
        src/google/protobuf/extension_set_unittest.cc
        src/google/protobuf/generated_message_reflection_unittest.cc
        src/google/protobuf/inlined_string_field_unittest.cc
        src/google/protobuf/io/coded_stream_unittest.cc
        src/google/protobuf/io/io_win32_unittest.cc
        src/google/protobuf/io/printer_unittest.cc
        src/google/protobuf/io/tokenizer_unittest.cc
        src/google/protobuf/io/zero_copy_stream_unittest.cc
        src/google/protobuf/map_field_test.cc
        src/google/protobuf/map_lite_test_util.cc
        src/google/protobuf/map_test.cc
        src/google/protobuf/map_test.inc
        src/google/protobuf/map_test_util.inc
        src/google/protobuf/message_unittest.cc
        src/google/protobuf/message_unittest.inc
        src/google/protobuf/no_field_presence_test.cc
        src/google/protobuf/preserve_unknown_enum_test.cc
        src/google/protobuf/proto3_arena_lite_unittest.cc
        src/google/protobuf/proto3_arena_unittest.cc
        src/google/protobuf/proto3_lite_unittest.cc
        src/google/protobuf/proto3_lite_unittest.inc
        src/google/protobuf/reflection_ops_unittest.cc
        src/google/protobuf/reflection_tester.cc
        src/google/protobuf/repeated_field_reflection_unittest.cc
        src/google/protobuf/repeated_field_unittest.cc
        src/google/protobuf/stubs/bytestream_unittest.cc
        src/google/protobuf/stubs/common_unittest.cc
        src/google/protobuf/stubs/int128_unittest.cc
        src/google/protobuf/stubs/status_test.cc
        src/google/protobuf/stubs/statusor_test.cc
        src/google/protobuf/stubs/stringpiece_unittest.cc
        src/google/protobuf/stubs/stringprintf_unittest.cc
        src/google/protobuf/stubs/structurally_valid_unittest.cc
        src/google/protobuf/stubs/strutil_unittest.cc
        src/google/protobuf/stubs/template_util_unittest.cc
        src/google/protobuf/stubs/time_test.cc
        src/google/protobuf/test_util.cc
        src/google/protobuf/test_util.inc
        src/google/protobuf/test_util_lite.cc
        src/google/protobuf/testing/file.cc
        src/google/protobuf/testing/googletest.cc
        src/google/protobuf/text_format_unittest.cc
        src/google/protobuf/unknown_field_set_unittest.cc
        src/google/protobuf/util/delimited_message_util_test.cc
        src/google/protobuf/util/field_comparator_test.cc
        src/google/protobuf/util/field_mask_util_test.cc
        src/google/protobuf/util/internal/default_value_objectwriter_test.cc
        src/google/protobuf/util/internal/json_objectwriter_test.cc
        src/google/protobuf/util/internal/json_stream_parser_test.cc
        src/google/protobuf/util/internal/protostream_objectsource_test.cc
        src/google/protobuf/util/internal/protostream_objectwriter_test.cc
        src/google/protobuf/util/internal/type_info_test_helper.cc
        src/google/protobuf/util/json_util_test.cc
        src/google/protobuf/util/message_differencer_unittest.cc
        src/google/protobuf/util/time_util_test.cc
        src/google/protobuf/util/type_resolver_util_test.cc
        src/google/protobuf/well_known_types_unittest.cc
        src/google/protobuf/wire_format_unittest.cc
        src/google/protobuf/wire_format_unittest.inc)
  target_link_libraries(protobuf_tests PRIVATE libprotoc libprotobuf zlib
                                               gmock_main)
  target_compile_options(
    protobuf_tests
    PRIVATE
      -DGOOGLE_PROTOBUF_TEST_PLUGIN_PATH="$<TARGET_FILE:protobuf_test_plugin>"
      -Wno-inconsistent-missing-override -Wno-deprecated-declarations)
  set_tests_properties(protobuf_tests PROPERTIES WORKING_DIRECTORY
                                                 ${protobuf_source_dir})
  add_dependencies(protobuf_tests protobuf_test_plugin)
endif()
