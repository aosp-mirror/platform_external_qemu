set(LIBCHROME_ROOT ${AOSP_ROOT}/external/libchrome)
enable_language(ASM)

# The libraries below are compiled against Win8 libs, with dynamic loading of
# newer apis.
if(WINDOWS_MSVC_X86_64)
  remove_definitions("-D_LIBCPP_VERSION=__GLIBCPP__")
  remove_definitions("-D_WIN32_WINNT=_WIN32_WINNT_WIN7")
  remove_definitions("-DNTDDI_VERSION=NTDDI_WIN7")
endif()

android_add_library(
  TARGET double-conversion
  LICENSE BSD-3-Clause
  SOURCE_DIR
    ${LIBCHROME_ROOT}/base/third_party/double_conversion/double-conversion
  SRC bignum-dtoa.cc
      bignum.cc
      cached-powers.cc
      double-to-string.cc
      fast-dtoa.cc
      fixed-dtoa.cc
      string-to-double.cc
      strtod.cc)

# static libchrome without allocators
android_add_library(
  TARGET libchrome
  LICENSE BSD-3-Clause
  SOURCE_DIR ${LIBCHROME_ROOT}
  SRC # cmake-format: sortable
      base/allocator/allocator_check.cc
      base/allocator/allocator_extension.cc
      base/allocator/allocator_shim_default_dispatch_to_partition_alloc.cc
      base/allocator/partition_allocator/address_pool_manager.cc
      base/allocator/partition_allocator/address_pool_manager_bitmap.cc
      base/allocator/partition_allocator/address_space_randomization.cc
      base/allocator/partition_allocator/extended_api.cc
      base/allocator/partition_allocator/memory_reclaimer.cc
      base/allocator/partition_allocator/oom_callback.cc
      base/allocator/partition_allocator/page_allocator.cc
      base/allocator/partition_allocator/partition_address_space.cc
      base/allocator/partition_allocator/partition_alloc.cc
      base/allocator/partition_allocator/partition_alloc_features.cc
      base/allocator/partition_allocator/partition_alloc_hooks.cc
      base/allocator/partition_allocator/partition_bucket.cc
      base/allocator/partition_allocator/partition_lock.cc
      base/allocator/partition_allocator/partition_oom.cc
      base/allocator/partition_allocator/partition_page.cc
      base/allocator/partition_allocator/partition_ref_count.cc
      base/allocator/partition_allocator/partition_root.cc
      base/allocator/partition_allocator/partition_stats.cc
      base/allocator/partition_allocator/pcscan.cc
      base/allocator/partition_allocator/random.cc
      base/allocator/partition_allocator/spinning_mutex.cc
      base/allocator/partition_allocator/thread_cache.cc
      base/at_exit.cc
      base/barrier_closure.cc
      # base/base64.cc # We don't want to bring in extra deps. base/base64url.cc
      base/base_paths.cc
      base/base_switches.cc # static library
      base/big_endian.cc
      # base/build_time.cc # Nope, no build time.
      base/callback_helpers.cc
      base/callback_internal.cc
      base/callback_list.cc
      base/check.cc
      base/check_op.cc
      base/command_line.cc
      base/containers/flat_tree.cc
      base/containers/intrusive_heap.cc
      base/containers/linked_list.cc
      base/cpu.cc
      base/debug/activity_analyzer.cc
      base/debug/activity_tracker.cc
      base/debug/alias.cc
      base/debug/asan_invalid_access.cc
      base/debug/crash_logging.cc
      base/debug/debugger.cc
      base/debug/dump_without_crashing.cc
      base/debug/profiler.cc
      base/debug/stack_trace.cc
      base/debug/task_trace.cc
      base/deferred_sequenced_task_runner.cc
      base/environment.cc
      base/feature_list.cc
      base/files/file.cc
      base/files/file_enumerator.cc
      base/files/file_path.cc
      base/files/file_path_constants.cc
      base/files/file_path_watcher.cc
      base/files/file_proxy.cc
      base/files/file_tracing.cc
      base/files/file_util.cc
      base/files/important_file_writer.cc
      base/files/important_file_writer_cleaner.cc
      base/files/memory_mapped_file.cc
      base/files/scoped_file.cc
      base/files/scoped_temp_dir.cc
      base/guid.cc
      base/hash/hash.cc
      base/hash/legacy_hash.cc
      base/hash/md5_boringssl.cc # we have boring ssl
      base/hash/sha1_boringssl.cc # we have boring ssl
      base/json/json_file_value_serializer.cc
      base/json/json_parser.cc
      base/json/json_reader.cc
      base/json/json_string_value_serializer.cc
      base/json/json_value_converter.cc
      base/json/json_writer.cc
      base/json/string_escape.cc
      base/lazy_instance_helpers.cc
      base/location.cc
      base/logging.cc
      base/memory/aligned_memory.cc
      base/memory/checked_ptr.cc
      base/memory/discardable_memory.cc
      base/memory/discardable_memory_allocator.cc
      base/memory/discardable_shared_memory.cc
      base/memory/memory_pressure_listener.cc
      base/memory/memory_pressure_monitor.cc
      base/memory/nonscannable_memory.cc
      base/memory/platform_shared_memory_region.cc
      base/memory/read_only_shared_memory_region.cc
      base/memory/ref_counted.cc
      base/memory/ref_counted_memory.cc
      base/memory/shared_memory_mapping.cc
      base/memory/shared_memory_security_policy.cc
      base/memory/shared_memory_tracker.cc
      base/memory/tagging.cc
      base/memory/unsafe_shared_memory_pool.cc
      base/memory/unsafe_shared_memory_region.cc
      base/memory/weak_ptr.cc
      base/memory/writable_shared_memory_region.cc
      base/message_loop/message_pump.cc
      base/message_loop/message_pump_default.cc
      base/message_loop/work_id_provider.cc
      base/metrics/bucket_ranges.cc
      base/metrics/crc32.cc
      base/metrics/dummy_histogram.cc
      base/metrics/field_trial.cc
      base/metrics/field_trial_param_associator.cc
      base/metrics/field_trial_params.cc
      base/metrics/histogram.cc
      base/metrics/histogram_base.cc
      base/metrics/histogram_delta_serialization.cc
      base/metrics/histogram_functions.cc
      base/metrics/histogram_samples.cc
      base/metrics/histogram_snapshot_manager.cc
      base/metrics/metrics_hashes.cc
      base/metrics/persistent_histogram_allocator.cc
      base/metrics/persistent_histogram_storage.cc
      base/metrics/persistent_memory_allocator.cc
      base/metrics/persistent_sample_map.cc
      base/metrics/sample_map.cc
      base/metrics/sample_vector.cc
      base/metrics/single_sample_metrics.cc
      base/metrics/sparse_histogram.cc
      base/metrics/statistics_recorder.cc
      base/metrics/user_metrics.cc
      base/native_library.cc
      base/notreached.cc
      base/observer_list_internal.cc
      base/observer_list_threadsafe.cc
      base/observer_list_types.cc
      base/one_shot_event.cc
      # base/os_compat_nacl.cc # nope , no nacl.
      base/path_service.cc
      base/pending_task.cc
      base/pickle.cc
      base/power_monitor/power_monitor.cc
      base/power_monitor/power_monitor_device_source.cc
      base/power_monitor/power_monitor_source.cc
      base/process/environment_internal.cc
      base/process/kill.cc
      base/process/launch.cc
      base/process/memory.cc
      base/process/process_handle.cc
      base/process/process_iterator.cc
      base/process/process_metrics.cc
      base/profiler/arm_cfi_table.cc
      base/profiler/frame.cc
      base/profiler/metadata_recorder.cc
      base/profiler/module_cache.cc
      base/profiler/sample_metadata.cc
      base/profiler/sampling_profiler_thread_token.cc
      base/profiler/stack_buffer.cc
      base/profiler/stack_copier.cc
      base/profiler/stack_copier_suspend.cc
      base/profiler/stack_sampler.cc
      base/profiler/stack_sampler_impl.cc
      base/profiler/stack_sampling_profiler.cc
      base/profiler/unwinder.cc
      base/rand_util.cc
      # base/rand_util_nacl.cc # No NaCL
      base/run_loop.cc
      base/sampling_heap_profiler/lock_free_address_hash_set.cc
      base/sampling_heap_profiler/poisson_allocation_sampler.cc
      base/sampling_heap_profiler/sampling_heap_profiler.cc
      base/scoped_native_library.cc
      base/sequence_checker_impl.cc
      base/sequence_token.cc
      base/sequenced_task_runner.cc
      base/strings/abseil_string_conversions.cc
      base/strings/escape.cc
      base/strings/latin1_string_conversions.cc
      base/strings/nullable_string16.cc
      base/strings/pattern.cc
      base/strings/safe_sprintf.cc
      base/strings/strcat.cc
      base/strings/string_number_conversions.cc
      base/strings/string_piece.cc
      base/strings/string_split.cc
      base/strings/string_util.cc
      base/strings/string_util_constants.cc
      base/strings/stringprintf.cc
      base/strings/utf_offset_string_conversions.cc
      base/strings/utf_string_conversion_utils.cc
      base/strings/utf_string_conversions.cc
      base/supports_user_data.cc
      base/sync_socket.cc
      base/synchronization/atomic_flag.cc
      base/synchronization/lock.cc
      base/syslog_logging.cc
      base/system/sys_info.cc
      base/system/system_monitor.cc
      base/task/cancelable_task_tracker.cc
      base/task/common/checked_lock_impl.cc
      base/task/common/operations_controller.cc
      base/task/common/scoped_defer_task_posting.cc
      base/task/common/task_annotator.cc
      base/task/current_thread.cc
      base/task/lazy_thread_pool_task_runner.cc
      base/task/post_job.cc
      base/task/post_task.cc
      base/task/scoped_set_task_priority_for_current_thread.cc
      base/task/sequence_manager/associated_thread_id.cc
      base/task/sequence_manager/atomic_flag_set.cc
      base/task/sequence_manager/enqueue_order_generator.cc
      base/task/sequence_manager/lazy_now.cc
      base/task/sequence_manager/real_time_domain.cc
      base/task/sequence_manager/sequence_manager.cc
      base/task/sequence_manager/sequence_manager_impl.cc
      base/task/sequence_manager/task_queue.cc
      base/task/sequence_manager/task_queue_impl.cc
      base/task/sequence_manager/task_queue_selector.cc
      base/task/sequence_manager/tasks.cc
      base/task/sequence_manager/thread_controller.cc
      base/task/sequence_manager/thread_controller_impl.cc
      base/task/sequence_manager/thread_controller_power_monitor.cc
      base/task/sequence_manager/thread_controller_with_message_pump_impl.cc
      base/task/sequence_manager/time_domain.cc
      base/task/sequence_manager/work_deduplicator.cc
      base/task/sequence_manager/work_queue.cc
      base/task/sequence_manager/work_queue_sets.cc
      base/task/simple_task_executor.cc
      base/task/single_thread_task_executor.cc
      base/task/task_executor.cc
      base/task/task_features.cc
      base/task/task_traits.cc
      base/task/thread_pool.cc
      base/task/thread_pool/delayed_task_manager.cc
      base/task/thread_pool/environment_config.cc
      base/task/thread_pool/initialization_util.cc
      base/task/thread_pool/job_task_source.cc
      base/task/thread_pool/pooled_parallel_task_runner.cc
      base/task/thread_pool/pooled_sequenced_task_runner.cc
      base/task/thread_pool/pooled_single_thread_task_runner_manager.cc
      base/task/thread_pool/pooled_task_runner_delegate.cc
      base/task/thread_pool/priority_queue.cc
      base/task/thread_pool/sequence.cc
      base/task/thread_pool/service_thread.cc
      base/task/thread_pool/task.cc
      base/task/thread_pool/task_source.cc
      base/task/thread_pool/task_source_sort_key.cc
      base/task/thread_pool/task_tracker.cc
      base/task/thread_pool/thread_group.cc
      base/task/thread_pool/thread_group_impl.cc
      base/task/thread_pool/thread_group_native.cc
      base/task/thread_pool/thread_pool_impl.cc
      base/task/thread_pool/thread_pool_instance.cc
      base/task/thread_pool/worker_thread.cc
      base/task/thread_pool/worker_thread_stack.cc
      base/task_runner.cc
      base/third_party/cityhash/city.cc
      base/third_party/cityhash_v103/src/city_v103.cc
      base/third_party/nspr/prtime.cc
      base/third_party/superfasthash/superfasthash.c
      base/threading/hang_watcher.cc
      base/threading/platform_thread.cc
      base/threading/post_task_and_reply_impl.cc
      base/threading/scoped_blocking_call.cc
      base/threading/scoped_blocking_call_internal.cc
      base/threading/scoped_thread_priority.cc
      base/threading/sequence_local_storage_map.cc
      base/threading/sequence_local_storage_slot.cc
      base/threading/sequenced_task_runner_handle.cc
      base/threading/simple_thread.cc
      base/threading/thread.cc
      base/threading/thread_checker_impl.cc
      base/threading/thread_collision_warner.cc
      base/threading/thread_id_name_manager.cc
      base/threading/thread_local_storage.cc
      base/threading/thread_restrictions.cc
      base/threading/thread_task_runner_handle.cc
      base/threading/watchdog.cc
      base/time/clock.cc
      base/time/default_clock.cc
      base/time/default_tick_clock.cc
      base/time/tick_clock.cc
      base/time/time.cc
      base/time/time_override.cc
      base/time/time_to_iso8601.cc
      base/timer/elapsed_timer.cc
      base/timer/lap_timer.cc
      base/timer/timer.cc
      base/token.cc
      base/trace_event/heap_profiler_allocation_context.cc
      base/trace_event/heap_profiler_allocation_context_tracker.cc
      base/trace_event/memory_allocator_dump_guid.cc
      base/trace_event/trace_event_stub.cc # No tracing.
      base/trace_event/trace_id_helper.cc
      base/unguessable_token.cc
      base/value_iterators.cc
      base/values.cc
      base/version.cc
      base/vlog.cc
  POSIX # cmake-format: sortable
        base/allocator/allocator_shim.cc
        base/allocator/partition_allocator/page_allocator_internals_posix.cc
        base/debug/debugger_posix.cc
        base/debug/stack_trace_posix.cc
        base/file_descriptor_store.cc
        base/files/file_descriptor_watcher_posix.cc
        base/files/file_enumerator_posix.cc
        base/files/file_posix.cc
        base/files/file_util_posix.cc
        base/files/memory_mapped_file_posix.cc
        base/memory/madv_free_discardable_memory_allocator_posix.cc
        base/memory/madv_free_discardable_memory_posix.cc
        base/message_loop/watchable_io_message_pump_posix.cc
        base/posix/file_descriptor_shuffle.cc
        base/posix/global_descriptors.cc
        base/posix/safe_strerror.cc
        base/posix/unix_domain_socket.cc
        base/process/kill_posix.cc
        base/process/process_handle_posix.cc
        base/process/process_metrics_posix.cc
        base/process/process_posix.cc
        base/rand_util_posix.cc
        base/sync_socket_posix.cc
        base/synchronization/condition_variable_posix.cc
        base/synchronization/lock_impl_posix.cc
        base/system/sys_info_posix.cc
        base/task/thread_pool/task_tracker_posix.cc
        base/threading/platform_thread_posix.cc
        base/threading/thread_local_storage_posix.cc
        base/time/time_conversion_posix.cc
        base/timer/hi_res_timer_manager_posix.cc
  LINUX # cmake-format: sortable
        base/allocator/allocator_shim_default_dispatch_to_glibc.cc
        base/base_paths_posix.cc
        base/cpu_affinity_posix.cc
        base/debug/elf_reader.cc
        base/debug/proc_maps_linux.cc
        base/files/file_path_watcher_linux.cc
        base/files/file_util_linux.cc
        base/memory/platform_shared_memory_region_posix.cc
        base/message_loop/message_pump_glib.cc
        base/message_loop/message_pump_libevent.cc
        base/native_library_posix.cc # linux
        # base/nix_xdg.cc
        base/posix/can_lower_nice_to.cc
        base/power_monitor/power_monitor_device_source_stub.cc
        base/process/internal_linux.cc
        base/process/launch_posix.cc # linux
        base/process/memory_linux.cc
        base/process/process_handle_linux.cc
        base/process/process_iterator_linux.cc
        base/process/process_linux.cc
        base/process/process_metrics_linux.cc
        base/profiler/module_cache_posix.cc # linux
        base/profiler/stack_copier_signal.cc
        base/profiler/stack_sampler_posix.cc
        base/profiler/thread_delegate_posix.cc
        base/strings/sys_string_conversions_posix.cc # linux
        base/synchronization/waitable_event_posix.cc # linux
        base/synchronization/waitable_event_watcher_posix.cc # linux
        base/system/sys_info_linux.cc
        base/threading/platform_thread_internal_posix.cc # linux
        base/threading/platform_thread_linux.cc
        # base/time/time_exploded_icu.cc
        base/time/time_exploded_posix.cc
        base/time/time_now_posix.cc
        # base/xdg_util.cc
  DARWIN # cmake-format: sortable
         base/allocator/allocator_interception_mac.mm
         base/allocator/allocator_shim_default_dispatch_to_mac_zoned_malloc.cc
         base/allocator/malloc_zone_functions_mac.cc
         base/base_paths_mac.mm
         base/enterprise_util_mac.mm
         base/file_version_info_mac.mm
         base/files/file_path_watcher_fsevents.cc
         base/files/file_path_watcher_kqueue.cc
         base/files/file_path_watcher_mac.cc
         base/files/file_util_mac.mm
         base/mac/authorization_util.mm
         base/mac/bundle_locations.mm
         base/mac/call_with_eh_frame.cc
         base/mac/call_with_eh_frame_asm.S
         base/mac/close_nocancel.cc
         base/mac/dispatch_source_mach.cc
         base/mac/foundation_util.mm
         base/mac/launch_services_util.mm
         base/mac/launchd.cc
         base/mac/mac_logging.mm
         base/mac/mac_util.mm
         base/mac/mach_logging.cc
         base/mac/mach_o.cc
         base/mac/mach_port_rendezvous.cc
         base/mac/objc_release_properties.mm
         base/mac/os_crash_dumps.cc
         base/mac/rosetta.mm
         base/mac/scoped_authorizationref.mm
         base/mac/scoped_mach_port.cc
         base/mac/scoped_mach_vm.cc
         base/mac/scoped_nsautorelease_pool.mm
         base/mac/scoped_objc_class_swizzler.mm
         base/mac/scoped_sending_event.mm
         base/mac/sdk_forward_declarations.mm
         base/memory/platform_shared_memory_region_mac.cc
         base/message_loop/message_pump_kqueue.cc
         base/message_loop/message_pump_mac.mm
         base/native_library_mac.mm
         base/power_monitor/power_monitor_device_source_mac.mm
         base/power_monitor/thermal_state_observer_mac.mm
         base/process/kill_mac.cc
         base/process/launch_mac.cc
         base/process/memory_mac.mm
         base/process/port_provider_mac.cc
         base/process/process_handle_mac.cc
         base/process/process_iterator_mac.cc
         base/process/process_mac.cc
         base/process/process_metrics_mac.cc
         base/profiler/module_cache_mac.cc
         base/profiler/native_unwinder_mac.cc
         base/profiler/stack_sampler_mac.cc
         base/profiler/suspendable_thread_delegate_mac.cc
         base/strings/sys_string_conversions_mac.mm
         base/synchronization/waitable_event_mac.cc
         base/synchronization/waitable_event_watcher_mac.cc
         base/system/sys_info_mac.mm
         base/task/thread_pool/thread_group_native_mac.mm
         base/threading/platform_thread_mac.mm
         base/time/time_exploded_posix.cc
         base/time/time_mac.cc
  WINDOWS # cmake-format: sortable
          base/allocator/allocator_shim_default_dispatch_to_winheap.cc
          base/allocator/partition_allocator/address_space_randomization.cc
          base/allocator/partition_allocator/oom_callback.cc
          base/allocator/partition_allocator/page_allocator.cc
          base/allocator/partition_allocator/partition_tls_win.cc
          base/allocator/partition_allocator/random.cc
          # base/allocator/winheap_stubs_win.cc # Allocator shim
          base/base_paths_win.cc
          base/debug/close_handle_hook_win.cc
          base/debug/debugger_win.cc
          base/debug/gdi_debug_util_win.cc
          base/debug/invalid_access_win.cc
          base/debug/stack_trace_win.cc
          base/enterprise_util_win.cc
          base/file_version_info_win.cc
          base/files/file_enumerator_win.cc
          base/files/file_path_watcher_win.cc
          base/files/file_util_win.cc
          base/files/file_win.cc
          base/files/memory_mapped_file_win.cc
          base/logging_win.cc
          base/memory/platform_shared_memory_region_win.cc
          base/message_loop/message_pump_win.cc
          base/native_library_win.cc
          base/power_monitor/power_monitor_device_source_win.cc
          base/process/kill_win.cc
          # base/process/launch_win.cc
          base/process/memory_win.cc
          base/process/process_handle_win.cc
          base/process/process_info_win.cc
          base/process/process_iterator_win.cc
          base/process/process_metrics_win.cc
          base/process/process_win.cc
          base/profiler/module_cache_win.cc
          base/profiler/native_unwinder_win.cc
          base/profiler/stack_sampler_win.cc
          base/profiler/suspendable_thread_delegate_win.cc
          base/profiler/win32_stack_frame_unwinder.cc
          base/rand_util_win.cc
          base/scoped_clear_last_error_win.cc
          base/strings/strcat_win.cc
          base/strings/string_number_conversions_win.cc
          base/strings/string_split_win.cc
          base/strings/string_util_win.cc
          base/strings/sys_string_conversions_win.cc
          base/sync_socket_win.cc
          base/synchronization/condition_variable_win.cc
          base/synchronization/lock_impl_win.cc
          base/synchronization/waitable_event_watcher_win.cc
          base/synchronization/waitable_event_win.cc
          base/system/sys_info_win.cc
          base/task/thread_pool/thread_group_native_win.cc
          base/threading/platform_thread_win.cc
          base/threading/thread_local_storage_win.cc
          base/time/time_win.cc
          base/timer/hi_res_timer_manager_win.cc
          base/win/atl_throw.cc
          base/win/com_init_balancer.cc
          base/win/com_init_check_hook.cc
          base/win/com_init_util.cc
          base/win/core_winrt_util.cc
          base/win/embedded_i18n/language_selector.cc
          base/win/enum_variant.cc
          base/win/event_trace_controller.cc
          base/win/event_trace_provider.cc
          base/win/hstring_compare.cc
          base/win/hstring_reference.cc
          base/win/i18n.cc
          base/win/iat_patch_function.cc
          base/win/message_window.cc
          base/win/object_watcher.cc
          base/win/patch_util.cc
          base/win/pe_image.cc
          base/win/pe_image_reader.cc
          base/win/process_startup_helper.cc
          base/win/registry.cc
          base/win/resource_util.cc
          base/win/scoped_bstr.cc
          base/win/scoped_com_initializer.cc
          base/win/scoped_handle.cc
          base/win/scoped_handle_verifier.cc
          base/win/scoped_hstring.cc
          base/win/scoped_process_information.cc
          base/win/scoped_variant.cc
          base/win/scoped_winrt_initializer.cc
          base/win/shortcut.cc
          base/win/startup_information.cc
          base/win/static_constants.cc # static lib
          base/win/variant_vector.cc
          base/win/vector.cc
          base/win/win_util.cc
          base/win/windows_defines.inc
          base/win/windows_undefines.inc
          base/win/windows_version.cc
          base/win/winrt_storage_util.cc
          base/win/wmi.cc
          base/win/wrapped_window_proc.cc)

# android_target_include_directories(libchrome darwin-x86_64 PRIVATE posix})
target_include_directories(
  libchrome PUBLIC . ${LIBCHROME_ROOT} ${CMAKE_CURRENT_BINARY_DIR} ${AOSP_ROOT}/external/webrtc
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android)

target_link_libraries(libchrome PUBLIC gtest absl::variant ssl absl::strings
                                       perfetto-tracing-only double-conversion)
target_compile_definitions(libchrome PUBLIC -D__ANDROID_HOST__
                                            -D__ANDROID_EMULATOR__)
target_compile_options(libchrome PRIVATE -Wno-undefined-var-template
                                         -Wno-c++11-narrowing)
android_target_compile_definitions(
  libchrome darwin-x86_64 PUBLIC -DOS_MAC=1 -DOS_APPLE=1 -DSYSTEM_NATIVE_UTF8)
android_target_compile_definitions(
  libchrome darwin-aarch64 PUBLIC -DOS_MAC=1 -DOS_APPLE=1 -DSYSTEM_NATIVE_UTF8)

if(LINUX_X86_64 OR LINUX_AARCH64)
  android_add_library(
    TARGET symbolize
    LICENSE BSD-3-Clause
    SOURCE_DIR ${LIBCHROME_ROOT}/base/third_party/symbolize
    SRC # cmake-format: sortable
        demangle.cc symbolize.cc)
  target_include_directories(symbolize PUBLIC ${LIBCHROME_ROOT}
                                              ${LIBCHROME_ROOT}/base/symbolize)
  # Workaround for our (old) linux headers.
  set_property(SOURCE ${LIBCHROME_ROOT}/base/rand_util_posix.cc
               PROPERTY COMPILE_DEFINITIONS OS_NACL=1)
  set_property(SOURCE ${LIBCHROME_ROOT}/base/process/memory_linux.cc
               PROPERTY COMPILE_DEFINITIONS __libc_malloc=malloc)
endif()
android_target_compile_definitions(libchrome linux-x86_64
                                   PUBLIC -DOS_UNIX=1 -DUSE_SYMBOLIZE)
android_target_compile_definitions(libchrome linux-aarch64
                                   PUBLIC -DOS_UNIX=1 -DUSE_SYMBOLIZE)
android_target_compile_options(libchrome linux-aarch64
                               PRIVATE -march=armv8-a+memtag)
# We explicitly set the windows version to be Win10 when compiling libchrome.
# libchrome will run on Win7, but needs compile time bindings to APIs that are
# not available in Win7. libchrome will load these functions dynamically when
# needed.
android_target_compile_definitions(
  libchrome windows
  PRIVATE
    -DOS_WIN=1
    -DUNICODE=1
    -D_UNICODE=1
    -D_ENABLE_ATOMIC_ALIGNMENT_FIX=1
    # See
    # https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers
    "-DNTDDI_VERSION=NTDDI_WIN10_RS2"
    "-D_WIN32_WINNT=_WIN32_WINNT_WIN10")

android_target_link_libraries(libchrome linux-x86_64 PRIVATE glib2
                                                             symbolize libevent)
android_target_link_libraries(libchrome linux-aarch64 PRIVATE glib2
                                                             symbolize libevent -ldl)

android_target_link_libraries(
  libchrome darwin-x86_64
  PRIVATE "-framework Foundation"
          "-framework CoreGraphics"
          "-framework IOKit"
          "-framework CoreText"
          "-framework Security"
          "-framework AppKit"
          bsm)

android_target_link_libraries(
  libchrome darwin-aarch64
  PRIVATE "-framework Foundation" "-framework CoreGraphics" "-framework IOKit"
          "-framework CoreText" "-framework Security" "-framework AppKit" bsm)

android_target_link_libraries(
  libchrome windows
  PRIVATE dbghelp::dbghelp
          powrprof::powrprof
          cfgmgr32::cfgmgr32
          setupapi::setupapi
          version::version
          propsys::propsys
          shlwapi::shlwapi
          delayimp::delayimp)

# Create a symlink so libchrome can find required includes, we already have all
# the required dependencies in our tree.
message(
  "Creating libchrome symlinks ${CMAKE_CURRENT_BINARY_DIR}/third_party -> ${AOSP_ROOT}/external"
)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${AOSP_ROOT}/external"
          "third_party" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
