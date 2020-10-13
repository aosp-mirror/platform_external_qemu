cmake_minimum_required(VERSION 3.5)

set(perfetto "${ANDROID_QEMU2_TOP_DIR}/../perfetto")

# Add the full perfetto_trace proto library. This is necessary to
# post-process traces with the full Perfetto tracing SDK
add_subdirectory(proto)

android_add_library(
  TARGET perfetto-tracing-only-dummy
  LIBNAME libperfetto-tracing-only-dummy
  LICENSE Apache
  SRC
  dummy.cc)

android_license(
  TARGET perfetto-libperfettobase
  LIBNAME libperfettobase
  URL "https://android.googlesource.com/platform/external/perfetto/+/refs/heads/master/src/base/"
  SPDX "Apache 2.0"
  LICENSE
    "https://android.googlesource.com/platform/external/perfetto/+/refs/heads/master/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/../perfetto/LICENSE")

android_add_library(
  TARGET perfetto-libperfettobase
  LIBNAME libperfettobase
  LICENSE Apache
  SRC
  ${perfetto}/src/base/logging.cc
  ${perfetto}/src/base/metatrace.cc
  ${perfetto}/src/base/paged_memory.cc
  ${perfetto}/src/base/string_splitter.cc
  ${perfetto}/src/base/string_utils.cc
  ${perfetto}/src/base/string_view.cc
  ${perfetto}/src/base/subprocess.cc
  ${perfetto}/src/base/thread_checker.cc
  ${perfetto}/src/base/time.cc
  ${perfetto}/src/base/uuid.cc
  ${perfetto}/src/base/virtual_destructors.cc
  ${perfetto}/src/base/waitable_event.cc
  ${perfetto}/src/base/watchdog_posix.cc)

target_include_directories(
    perfetto-libperfettobase PUBLIC
    ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only
    ${perfetto}/include)

android_add_executable(
  TARGET perfetto-protozero-protoc-plugin INSTALL .
  LICENSE Apache
  LIBNAME
  libprotozero
  URL "https://android.googlesource.com/platform/external/perfetto/+/refs/heads/master/src/protozero/protoc_plugin"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../perfetto/src/protozero/protoc_plugin"
  NOTICE "REPO/LICENSE"
  SRC ${perfetto}/src/protozero/protoc_plugin/protozero_plugin.cc)

target_link_libraries(perfetto-protozero-protoc-plugin PRIVATE perfetto-libperfettobase libprotoc libprotobuf)

android_license(
  TARGET perfetto-libprotozero
  LIBNAME libprotozero
  URL "https://android.googlesource.com/platform/external/perfetto/+/refs/heads/master/src/protozero/"
  SPDX "Apache 2.0"
  LICENSE
    "https://android.googlesource.com/platform/external/perfetto/+/refs/heads/master/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/../perfetto/LICENSE")

android_add_library(
  TARGET perfetto-libprotozero
  LIBNAME libprotozero
  LICENSE Apache
  SRC
  ${perfetto}/src/protozero/field.cc
  ${perfetto}/src/protozero/message.cc
  ${perfetto}/src/protozero/message_arena.cc
  ${perfetto}/src/protozero/message_handle.cc
  ${perfetto}/src/protozero/packed_repeated_fields.cc
  ${perfetto}/src/protozero/proto_decoder.cc
  ${perfetto}/src/protozero/scattered_heap_buffer.cc
  ${perfetto}/src/protozero/scattered_stream_null_delegate.cc
  ${perfetto}/src/protozero/scattered_stream_writer.cc
  ${perfetto}/src/protozero/static_buffer.cc
  ${perfetto}/src/protozero/virtual_destructors.cc)

target_include_directories(
    perfetto-libprotozero PUBLIC
    ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only
    ${perfetto}/include)

target_link_libraries(
    perfetto-libprotozero PUBLIC perfetto-libperfettobase)

function(android_add_pbzero_library directory name)
    set(android_add_pbzero_library_protosrc "${directory}/${name}.proto")
    set(android_add_pbzero_library_protosrc_generated "${name}.pbzero.cc")
    get_filename_component(android_add_pbzero_library_protosrc_absdir ${directory} ABSOLUTE)

    if (MSVC)
        set(EXE_SUFFIX ".exe")
    else()
        set(EXE_SUFFIX "")
    endif()

    message("Adding pbzero for ${name}. Tracing protobuf src absdir: ${android_add_pbzero_library_protosrc_absdir}")

    android_add_protobuf_with_plugin(
      pbzero-${name}
      .pbzero.h
      "protoc-gen-plugin=${CMAKE_BINARY_DIR}/perfetto-protozero-protoc-plugin${EXE_SUFFIX}"
      "wrapper_namespace=pbzero:${CMAKE_CURRENT_BINARY_DIR}"
      perfetto-libprotozero
      "-I ${perfetto} -I ${android_add_pbzero_library_protosrc_absdir}"
      "${android_add_pbzero_library_protosrc}"
      "${android_add_pbzero_library_protosrc_generated}")

    add_dependencies(
      pbzero-${name}
      perfetto-protozero-protoc-plugin)
endfunction()

android_add_pbzero_library(${perfetto}/protos/perfetto/common trace_stats)
android_add_pbzero_library(${perfetto}/protos/perfetto/config trace_config)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace trace_packet)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace trace)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace clock_snapshot)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/interned_data interned_data)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/track_event counter_descriptor)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/track_event process_descriptor)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/track_event thread_descriptor)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/track_event track_descriptor)
android_add_pbzero_library(${perfetto}/protos/perfetto/trace/track_event track_event)

if (OPTION_PERFETTO_USE_SDK)
    set(PERFETTO_TRACING_ONLY_SOURCES
        perfetto.cc
        perfetto-sdk-tracing-only.cpp)
    set(PERFETTO_TRACING_ONLY_INCLUDE
        ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only)
    set(PERFETTO_TRACING_ONLY_LIBRARIES
        perfetto-tracing-only-dummy)
else()
    set(PERFETTO_TRACING_ONLY_SOURCES
        perfetto-tracing-only.cpp)
    set(PERFETTO_TRACING_ONLY_INCLUDE
        ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only
        ${perfetto}/include)
    set(PERFETTO_TRACING_ONLY_LIBRARIES
        pbzero-trace_stats
        pbzero-trace_config
        pbzero-clock_snapshot
        pbzero-trace
        pbzero-trace_packet
        pbzero-interned_data
        pbzero-process_descriptor
        pbzero-counter_descriptor
        pbzero-thread_descriptor
        pbzero-track_descriptor
        pbzero-track_event)
endif()

android_add_library(
    TARGET perfetto-tracing-only
    LICENSE Apache-2.0
    SRC
    ${PERFETTO_TRACING_ONLY_SOURCES})
target_include_directories(
    perfetto-tracing-only PUBLIC
    ${PERFETTO_TRACING_ONLY_INCLUDE})
target_link_libraries(perfetto-tracing-only
    PRIVATE
    ${PERFETTO_TRACING_ONLY_LIBRARIES})

android_add_test(
    TARGET perfetto_unittests
    SRC perfetto_unittest.cpp)
target_link_libraries(perfetto_unittests PUBLIC perfetto-tracing-only gtest gtest_main)

