android_add_library(
  TARGET crashpad_minidump_format
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      minidump_extensions.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_link_libraries(crashpad_minidump_format PUBLIC crashpad_compat
                      PRIVATE crashpad_util crashpad_snapshot)

android_add_library(
  TARGET crashpad_minidump
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      minidump_annotation_writer.cc
      minidump_byte_array_writer.cc
      minidump_context_writer.cc
      minidump_crashpad_info_writer.cc
      minidump_exception_writer.cc
      minidump_file_writer.cc
      minidump_handle_writer.cc
      minidump_memory_info_writer.cc
      minidump_memory_writer.cc
      minidump_misc_info_writer.cc
      minidump_module_crashpad_info_writer.cc
      minidump_module_writer.cc
      minidump_rva_list_writer.cc
      minidump_simple_string_dictionary_writer.cc
      minidump_stream_writer.cc
      minidump_string_writer.cc
      minidump_system_info_writer.cc
      minidump_thread_id_map.cc
      minidump_thread_writer.cc
      minidump_unloaded_module_writer.cc
      minidump_user_extension_stream_data_source.cc
      minidump_user_stream_writer.cc
      minidump_writable.cc
      minidump_writer_util.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_link_libraries(
  crashpad_minidump PUBLIC crashpad_compat crashpad_minidump_format
  PRIVATE crashpad_util crashpad_snapshot crashpad_context)

android_add_library(
  TARGET crashpad_minidump_test_support
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      test/minidump_byte_array_writer_test_util.cc
      test/minidump_context_test_util.cc
      test/minidump_file_writer_test_util.cc
      test/minidump_memory_writer_test_util.cc
      test/minidump_rva_list_test_util.cc
      test/minidump_string_writer_test_util.cc
      test/minidump_user_extension_stream_util.cc
      test/minidump_writable_test_util.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_link_libraries(crashpad_minidump_test_support PUBLIC crashpad_minidump
                      PRIVATE crashpad_util crashpad_snapshot crashpad_test  gmock)

android_add_test(
  TARGET crashpad_minidump_test
  SRC # cmake-format: sortable
      minidump_annotation_writer_test.cc
      minidump_byte_array_writer_test.cc
      minidump_context_writer_test.cc
      minidump_crashpad_info_writer_test.cc
      minidump_exception_writer_test.cc
      minidump_file_writer_test.cc
      minidump_handle_writer_test.cc
      minidump_memory_info_writer_test.cc
      minidump_memory_writer_test.cc
      minidump_misc_info_writer_test.cc
      minidump_module_crashpad_info_writer_test.cc
      minidump_module_writer_test.cc
      minidump_rva_list_writer_test.cc
      minidump_simple_string_dictionary_writer_test.cc
      minidump_string_writer_test.cc
      minidump_system_info_writer_test.cc
      minidump_thread_id_map_test.cc
      minidump_thread_writer_test.cc
      minidump_unloaded_module_writer_test.cc
      minidump_user_stream_writer_test.cc
      minidump_writable_test.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)
target_link_libraries(
  crashpad_minidump_test PRIVATE crashpad_handler_handler crashpad_minidump_test_support crashpad_snapshot_test_support  gmock
                                 crashpad_googlemock_main)
