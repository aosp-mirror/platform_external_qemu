enable_language(ASM)

# ~~~
# Mig magic!
#
# Crashpad generates a set of files based upon the current sdk.
# We pack this into a library to which we can link if needed.
#
# ``TARGET`` The name of the target
# ``SRC`` List of source files to be compiled.
# ~~~
function(android_mig_lib)
  # Parse arguments
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs SRC)
  cmake_parse_arguments(mig "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  set(mig_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/util/mach)
  if(NOT mig_SRC)
    message(FATAL_ERROR "Error: android_mig() called without any files")
    return()
  endif()

  set(mig_GEN "")
  if(DARWIN_AARCH64)
    set(mig_AARCH "arm")
  elseif(DARWIN_X86_64)
    set(mig_AARCH "x86_64")
  else()
    message(FATAL_ERROR "Not targeting darwin?!")
  endif()
  file(MAKE_DIRECTORY ${mig_OUTPUT_DIR})
  foreach(FIL ${mig_SRC})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    add_custom_command(
      OUTPUT "${mig_OUTPUT_DIR}/${FIL_WE}User.c"
             "${mig_OUTPUT_DIR}/${FIL_WE}Server.c"
             "${mig_OUTPUT_DIR}/${FIL_WE}.h"
             "${mig_OUTPUT_DIR}/${FIL_WE}Server.h"
      COMMAND
        python3 mach/mig.py "${ABS_FIL}" "${mig_OUTPUT_DIR}/${FIL_WE}User.c"
        "${mig_OUTPUT_DIR}/${FIL_WE}Server.c" "${mig_OUTPUT_DIR}/${FIL_WE}.h"
        "${mig_OUTPUT_DIR}/${FIL_WE}Server.h" --sdk ${CMAKE_OSX_SYSROOT}
        --include ../.. --include ../../compat/mac --arch ${mig_AARCH}
      COMMENT "Generating mig files from ${ABS_FILE}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      DEPENDS ${ABS_FIL})

    list(APPEND mig_GEN "${mig_OUTPUT_DIR}/${FIL_WE}User.c")
    list(APPEND mig_GEN "${mig_OUTPUT_DIR}/${FIL_WE}Server.c")
    list(APPEND mig_GEN "${mig_OUTPUT_DIR}/${FIL_WE}.h")
    list(APPEND mig_GEN "${mig_OUTPUT_DIR}/${FIL_WE}Server.h")
    set_source_files_properties(${mig_OUTPUT_DIR}/${FIL_WE}User.c
                                PROPERTIES GENERATED TRUE)
    set_source_files_properties(${mig_OUTPUT_DIR}/${FIL_WE}Server.c
                                PROPERTIES GENERATED TRUE)
    set_source_files_properties(${mig_OUTPUT_DIR}/${FIL_WE}.h
                                PROPERTIES GENERATED TRUE)
    set_source_files_properties(${mig_OUTPUT_DIR}/${FIL_WE}Server.h
                                PROPERTIES GENERATED TRUE)
  endforeach()

  android_add_library(TARGET ${mig_TARGET} LICENSE Apache-2.0 SRC ${mig_GEN})
  target_include_directories(${mig_TARGET}
                             PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gen PRIVATE ..)
  # Make the library available
  if(mig_GENERATED)
    set(${mig_GENERATED} "${mig_GEN}" PARENT_SCOPE)
  endif()
endfunction()

android_add_library(
  TARGET crashpad_util
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      file/delimited_file_reader.cc
      file/file_helper.cc
      file/file_io.cc
      file/file_reader.cc
      file/file_seeker.cc
      file/file_writer.cc
      file/output_stream_file_writer.cc
      file/scoped_remove_file.cc
      file/string_file.cc
      misc/initialization_state_dcheck.cc
      misc/lexing.cc
      misc/metrics.cc
      misc/pdb_structures.cc
      misc/random_string.cc
      misc/range_set.cc
      misc/reinterpret_bytes.cc
      misc/scoped_forbid_return.cc
      misc/time.cc
      misc/uuid.cc
      misc/zlib.cc
      net/http_body.cc
      net/http_body_gzip.cc
      net/http_multipart_builder.cc
      net/http_transport.cc
      net/url.cc
      numeric/checked_address_range.cc
      process/process_memory.cc
      process/process_memory_range.cc
      stdlib/aligned_allocator.cc
      stdlib/string_number_conversion.cc
      stdlib/strlcpy.cc
      stdlib/strnlen.cc
      stream/base94_output_stream.cc
      stream/file_encoder.cc
      stream/file_output_stream.cc
      stream/log_output_stream.cc
      stream/zlib_output_stream.cc
      string/split_string.cc
      thread/thread.cc
      thread/thread_log_messages.cc
      thread/worker_thread.cc
  LINUX # cmake-format: sortable
        file/directory_reader_posix.cc
        file/file_io_posix.cc
        file/filesystem_posix.cc
        linux/auxiliary_vector.cc
        linux/direct_ptrace_connection.cc
        linux/exception_handler_client.cc
        linux/exception_handler_protocol.cc
        linux/memory_map.cc
        linux/proc_stat_reader.cc
        linux/proc_task_reader.cc
        linux/ptrace_broker.cc
        linux/ptrace_client.cc
        linux/ptracer.cc
        linux/scoped_pr_set_dumpable.cc
        linux/scoped_pr_set_ptracer.cc
        linux/scoped_ptrace_attach.cc
        linux/socket.cc
        linux/thread_info.cc
        misc/capture_context_linux.S
        misc/clock_posix.cc
        misc/paths_linux.cc
        misc/time_linux.cc
        net/http_transport_socket.cc
        posix/close_multiple.cc
        posix/close_stdio.cc
        posix/double_fork_and_exec.cc
        posix/drop_privileges.cc
        posix/process_info_linux.cc
        posix/scoped_dir.cc
        posix/scoped_mmap.cc
        posix/signals.cc
        posix/symbolic_constants_posix.cc
        process/process_memory_linux.cc
        process/process_memory_sanitized.cc
        synchronization/semaphore_posix.cc
        thread/thread_posix.cc
  DARWIN # cmake-format: sortable
         file/directory_reader_posix.cc
         file/file_io_posix.cc
         file/filesystem_posix.cc
         mac/launchd.mm
         mac/mac_util.cc
         mac/service_management.cc
         mac/sysctl.cc
         mac/xattr.cc
         mach/bootstrap.cc
         mach/child_port_handshake.cc
         mach/child_port_server.cc
         mach/composite_mach_message_server.cc
         mach/exc_client_variants.cc
         mach/exc_server_variants.cc
         mach/exception_behaviors.cc
         mach/exception_ports.cc
         mach/exception_types.cc
         mach/mach_extensions.cc
         mach/mach_message.cc
         mach/mach_message_server.cc
         mach/notify_server.cc
         mach/scoped_task_suspend.cc
         mach/symbolic_constants_mach.cc
         mach/task_for_pid.cc
         misc/capture_context_mac.S
         misc/clock_mac.cc
         misc/clock_posix.cc
         misc/paths_mac.cc
         net/http_transport_mac.mm
         posix/close_multiple.cc
         posix/close_stdio.cc
         posix/double_fork_and_exec.cc
         posix/drop_privileges.cc
         posix/process_info_mac.cc
         posix/scoped_dir.cc
         posix/scoped_mmap.cc
         posix/signals.cc
         posix/symbolic_constants_posix.cc
         process/process_memory_mac.cc
         synchronization/semaphore_mac.cc
         synchronization/semaphore_posix.cc
         thread/thread_posix.cc
  WINDOWS # cmake-format: sortable
          file/directory_reader_win.cc
          file/file_io_win.cc
          file/filesystem_win.cc
          misc/clock_win.cc
          misc/paths_win.cc
          misc/time_win.cc
          net/http_transport_win.cc
          process/process_memory_win.cc
          synchronization/semaphore_win.cc
          thread/thread_win.cc
          win/command_line.cc
          win/critical_section_with_debug_info.cc
          win/exception_handler_server.cc
          win/get_function.cc
          win/get_module_information.cc
          win/handle.cc
          win/initial_client_data.cc
          win/loader_lock.cc
          win/module_version.cc
          win/nt_internals.cc
          win/ntstatus_logging.cc
          win/process_info.cc
          win/registration_protocol_win.cc
          win/scoped_handle.cc
          win/scoped_local_alloc.cc
          win/scoped_process_suspend.cc
          win/scoped_set_event.cc
          win/session_end_watcher.cc
          win/xp_compat.h)

target_include_directories(crashpad_util PUBLIC ..)
target_link_libraries(crashpad_util PUBLIC mini_chromium crashpad_compat)

if(WINDOWS_MSVC_X86_64)

  # We have setup git to covert \n -> \r\n causing these tests to fail, these will succeed if you
  # have configued git properly.
  set(CRASHPAD_UTIL_TEST_FILTER
      "--gtest_filter=-FileReaderHTTPBodyStream.ReadASCIIFile:HTTPMultipartBuilder.ThreeFileAttachments:HTTPMultipartBuilder.OverwriteFileAttachment:VariableBufferSize*"
  )
  if(CROSSCOMPILE)
    android_nasm_compile(
      TARGET crashpad_util_asm_lib LICENSE APACHE-2.0 FLAGS "-t"
      SRC # cmake-format: sortable misc/capture_context_win.asm
          win/safe_terminate_process.asm)
    target_sources(crashpad_util
                   PRIVATE misc/capture_context_win_emu_cross_hack.cc)
  else()
    # We are going to use ML64.exe to compile these.
    enable_language(ASM_MASM)
    add_library(crashpad_util_asm_lib misc/capture_context_win.asm
                                      win/safe_terminate_process.asm)

    set_target_properties(crashpad_util_asm_lib PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  target_link_libraries(
    crashpad_util PUBLIC crashpad_util_asm_lib
    PRIVATE user32::user32 version::version winhttp::winhttp)
elseif(LINUX)
  # There is some bizarre configuration issue on our bots, preventing us from
  # setting thread priorities, these will pass on your dev box.
  set(CRASHPAD_UTIL_TEST_FILTER "--gtest_filter=-.*Thread.*")
  target_link_libraries(crashpad_util PRIVATE crypto ssl)
  target_compile_definitions(
    crashpad_util PUBLIC CRASHPAD_LSS_SOURCE_EMBEDDED CRASHPAD_USE_BORINGSSL
    PRIVATE "O_TMPFILE=(020000000 | O_DIRECTORY)")
else()
  # We are APPLE!
  android_mig_lib(
    TARGET
    crashpad_macos_mig
    SRC
    ${CMAKE_OSX_SYSROOT}/usr/include/mach/exc.defs
    ${CMAKE_OSX_SYSROOT}/usr/include/mach/mach_exc.defs
    ${CMAKE_OSX_SYSROOT}/usr/include/mach/notify.defs
    mach/child_port.defs)

  target_link_libraries(
    crashpad_util PRIVATE crashpad_macos_mig "-framework CoreFoundation"
                          "-framework Foundation" "-framework IOKit" bsm)

  # There is some bizarre configuration issue on our bots, which seems to cause
  # issues with the launchd service, these tests will succeed on your dev box.
  set(CRASHPAD_UTIL_TEST_FILTER
      "--gtest_filter=-ServiceManagement.SubmitRemoveJob:Bootstrap.BootstrapCheckInAndLookUp:Bootstrap.SystemCrashReporterHandler"
  )
endif()

target_compile_definitions(crashpad_util PUBLIC CRASHPAD_ZLIB_SOURCE_SYSTEM
                                                ZLIB_CONST)
target_link_libraries(crashpad_util PRIVATE crypto ssl PUBLIC zlib)

android_add_test(
  TARGET crashpad_util_test
  SRC # cmake-format: sortable
      file/delimited_file_reader_test.cc
      file/directory_reader_test.cc
      file/file_io_test.cc
      file/file_reader_test.cc
      file/filesystem_test.cc
      file/string_file_test.cc
      misc/arraysize_test.cc
      misc/capture_context_test.cc
      misc/clock_test.cc
      misc/from_pointer_cast_test.cc
      misc/initialization_state_dcheck_test.cc
      misc/initialization_state_test.cc
      misc/no_cfi_icall_test.cc
      misc/paths_test.cc
      misc/random_string_test.cc
      misc/range_set_test.cc
      misc/reinterpret_bytes_test.cc
      misc/scoped_forbid_return_test.cc
      misc/time_test.cc
      misc/uuid_test.cc
      net/http_body_gzip_test.cc
      net/http_body_test.cc
      net/http_body_test_util.cc
      net/http_multipart_builder_test.cc
      net/url_test.cc
      numeric/checked_address_range_test.cc
      numeric/checked_range_test.cc
      numeric/in_range_cast_test.cc
      numeric/int128_test.cc
      process/process_memory_range_test.cc
      process/process_memory_test.cc
      stdlib/aligned_allocator_test.cc
      stdlib/map_insert_test.cc
      stdlib/string_number_conversion_test.cc
      stdlib/strlcpy_test.cc
      stdlib/strnlen_test.cc
      stdlib/thread_safe_vector_test.cc
      stream/base94_output_stream_test.cc
      stream/file_encoder_test.cc
      stream/log_output_stream_test.cc
      stream/test_output_stream.cc
      stream/zlib_output_stream_test.cc
      string/split_string_test.cc
      synchronization/semaphore_test.cc
      thread/thread_log_messages_test.cc
      thread/thread_test.cc
      thread/worker_thread_test.cc
  LINUX # cmake-format: sortable
        linux/auxiliary_vector_test.cc
        linux/memory_map_test.cc
        linux/proc_stat_reader_test.cc
        linux/proc_task_reader_test.cc
        linux/ptrace_broker_test.cc
        linux/ptracer_test.cc
        linux/scoped_ptrace_attach_test.cc
        linux/socket_test.cc
        misc/capture_context_test_util_linux.cc
        posix/process_info_test.cc
        posix/scoped_mmap_test.cc
        posix/signals_test.cc
        posix/symbolic_constants_posix_test.cc
        process/process_memory_sanitized_test.cc
  DARWIN # cmake-format: sortable
         mac/launchd_test.mm
         mac/mac_util_test.mm
         mac/service_management_test.mm
         mac/sysctl_test.cc
         mac/xattr_test.cc
         mach/bootstrap_test.cc
         mach/child_port_handshake_test.cc
         mach/child_port_server_test.cc
         mach/composite_mach_message_server_test.cc
         mach/exc_client_variants_test.cc
         mach/exc_server_variants_test.cc
         mach/exception_behaviors_test.cc
         mach/exception_ports_test.cc
         mach/exception_types_test.cc
         mach/mach_extensions_test.cc
         mach/mach_message_server_test.cc
         mach/mach_message_test.cc
         mach/notify_server_test.cc
         mach/scoped_task_suspend_test.cc
         mach/symbolic_constants_mach_test.cc
         misc/capture_context_test_util_mac.cc
         process/process_memory_mac_test.cc
  WINDOWS # cmake-format: sortable
          misc/capture_context_test_util_win.cc
          win/command_line_test.cc
          win/critical_section_with_debug_info_test.cc
          win/exception_handler_server_test.cc
          win/get_function_test.cc
          win/handle_test.cc
          win/initial_client_data_test.cc
          win/loader_lock_test.cc
          win/process_info_test.cc
          win/registration_protocol_win_test.cc
          win/safe_terminate_process_test.cc
          win/scoped_process_suspend_test.cc
          win/session_end_watcher_test.cc
  TEST_PARAMS "${CRASHPAD_UTIL_TEST_FILTER}")
target_link_libraries(
  crashpad_util_test PRIVATE crashpad_util mini_chromium crashpad_test gmock
                             crashpad_googlemock_main UUID::UUID)

android_target_link_libraries(crashpad_util_test windows_msvc-x86_64
                              PRIVATE dbghelp::dbghelp)

if(WINDOWS_MSVC_X86_64)

  android_add_executable(TARGET crashpad_util_test_process_info_test_child
                         NODISTRIBUTE SRC win/process_info_test_child.cc)
  target_compile_definitions(crashpad_util_test_process_info_test_child
                             PRIVATE UNICODE)

  android_add_executable(
    TARGET crashpad_util_test_safe_terminate_process_test_child NODISTRIBUTE
    SRC win/safe_terminate_process_test_child.cc)

  add_library(crashpad_util_test_loader_lock_test SHARED
              win/loader_lock_test_dll.cc)
  target_link_libraries(crashpad_util_test_loader_lock_test
                        PRIVATE crashpad_util)
  set_target_properties(
    crashpad_util_test_loader_lock_test
    PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
set_property(
  TEST crashpad_util_test APPEND
  PROPERTY ENVIRONMENT "CRASHPAD_TEST_DATA_ROOT=${CRASHPAD_TEST_DATA_ROOT}")
