if(WINDOWS_MSVC_X86_64)
  # In windows we must generate a shared library, lest we face the wrath of
  # double deletes.
  android_add_library(TARGET metrics SHARED LICENSE Apache-2.0)
  protobuf_generate_with_plugin(
    TARGET metrics PROTOS google_logs_publishing.proto studio_stats.proto
    OUT_VAR AEMU_METRICS_SRC
    EXPORT_MACRO "AEMU_METRICS_PROTO")
  target_link_libraries(metrics PUBLIC libprotobuf)
  target_include_directories(metrics PRIVATE . PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

  # Make sure our dependents are using dll import on all the properties.
  target_compile_definitions(metrics PRIVATE PUBLIC AEMU_METRICS_PROTO=__declspec\(dllimport\))

  # Make sure that we export them when building the dll
  set_source_files_properties(${AEMU_METRICS_SRC} PROPERTIES COMPILE_DEFINITIONS  AEMU_METRICS_PROTO=__declspec\(dllexport\) )
  add_custom_command(
    TARGET metrics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:metrics>"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  android_install_shared(metrics)
else()
    set(metrics_src google_logs_publishing.proto studio_stats.proto)
    android_add_protobuf(metrics "${metrics_src}")
endif()
