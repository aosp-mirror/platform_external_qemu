# In order for these to be a proper shared library you must:
#
# * Figure out which functions in ./include/... need to be made public
# * Setup an export/import header
# * Make sure the protobufs are configured as shared libraries
#
# The library should already be self contained and can be made shared on linux
# with no changes.
android_add_library(
  TARGET android-emu-metrics
  SHARED
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/CommonReportedInfo.cpp
      android/metrics/AsyncMetricsReporter.cpp
      android/metrics/CrashMetricsReporting.cpp
      android/metrics/FileMetricsWriter.cpp
      android/metrics/metrics.cpp
      android/metrics/MetricsReporter.cpp
      android/metrics/MetricsWriter.cpp
      android/metrics/NullMetricsReporter.cpp
      android/metrics/NullMetricsWriter.cpp
      android/metrics/Percentiles.cpp
      android/metrics/PeriodicReporter.cpp
      android/metrics/PlaystoreMetricsWriter.cpp
      android/metrics/SyncMetricsReporter.cpp
      android/metrics/TextMetricsWriter.cpp
      android/metrics/UiEventTracker.cpp
      android/protobuf/DelimitedSerialization.cpp
      android/session_phase_reporter.cpp)

if(WINDOWS_MSVC_X86_64)

  protobuf_generate_with_plugin(
    TARGET android-emu-metrics PROTOS protos/google_logs_publishing.proto
                                      protos/studio_stats.proto
    OUT_VAR AEMU_METRICS_SRC EXPORT_MACRO "AEMU_METRICS_PROTO")
  set_source_files_properties(
    ${AEMU_METRICS_SRC} PROPERTIES COMPILE_DEFINITIONS
                                   AEMU_METRICS_PROTO=__declspec\(dllexport\))
  # Make sure our dependents are using dll import on all the properties.
  target_compile_definitions(android-emu-metrics
                             PUBLIC AEMU_METRICS_PROTO=__declspec\(dllimport\))
else()
  protobuf_generate_with_plugin(
    TARGET android-emu-metrics PROTOS protos/google_logs_publishing.proto
                                      protos/studio_stats.proto)
endif()

target_link_libraries(
  android-emu-metrics
  PRIVATE absl::memory
          android-emu-base
          android-emu-base-headers
          android-emu-avd
          android-emu-utils
          android-emu-cmdline
          android-emu-curl
          android-emu-files
          android-emu-feature
          android-hw-config
          picosha2
          LibXml2::LibXml2
          android-emu-agents
          qemu-host-common-headers
          protobuf::libprotobuf
          ui::common
  PUBLIC android-emu-files android-emu-studio-config)

target_compile_definitions(android-emu-metrics PRIVATE AEMU_METRICS_EXPORTS=1)
target_compile_options(android-emu-metrics PRIVATE -Wno-extern-c-compat
                                                   -Wno-return-type-c-linkage)
target_include_directories(
  android-emu-metrics
  PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/protos/
  PRIVATE
    src ${ANDROID_QEMU2_TOP_DIR}/android/android-emu
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})
android_install_shared_library(TARGET android-emu-metrics)

if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET android-emu-metrics_unittests
    SRC test/android/CommonReportedInfo_unittest.cpp
        test/android/metrics/tests/AsyncMetricsReporter_unittest.cpp
        test/android/metrics/tests/FileMetricsWriter_unittest.cpp
        test/android/metrics/tests/MetricsReporter_unittest.cpp
        test/android/metrics/tests/MockMetricsReporter.cpp
        test/android/metrics/tests/MockMetricsWriter.cpp
        test/android/metrics/tests/NullMetricsClasses_unittest.cpp
        test/android/metrics/tests/Percentiles_unittest.cpp
        test/android/metrics/tests/PeriodicReporter_unittest.cpp
        test/android/metrics/tests/PlaystoreMetricsWriter_unittest.cpp
        test/android/metrics/tests/SyncMetricsReporter_unittest.cpp
        test/android/metrics/tests/UiEventTracker_unittest.cpp
    WINDOWS # These classes are internal to the dll, so we just dupe them for
            # the unit tests
            src/android/metrics/AsyncMetricsReporter.cpp
            src/android/metrics/NullMetricsReporter.cpp
            src/android/metrics/NullMetricsWriter.cpp)

  target_compile_options(android-emu-metrics_unittests PRIVATE -O0)
  target_include_directories(android-emu-metrics_unittests PRIVATE src test)
  target_link_libraries(android-emu-metrics_unittests
                        PRIVATE gmock_main android-emu android-emu-base-headers android-emu-metrics)

endif()
