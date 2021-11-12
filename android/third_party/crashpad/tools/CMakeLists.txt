android_add_library(
  TARGET crashpad_tool_support
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      tool_support.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_include_directories(crashpad_tool_support PUBLIC ..)
target_link_libraries(crashpad_tool_support PUBLIC mini_chromium)

android_add_executable(
  TARGET crashpad_database_util
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      crashpad_database_util.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_link_libraries(
  crashpad_database_util PUBLIC crashpad_client crashpad_compat crashpad_util
                                crashpad_tool_support zlib)
