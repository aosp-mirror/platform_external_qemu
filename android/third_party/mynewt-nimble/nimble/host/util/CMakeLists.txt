add_library(_nimble_util src/addr.c)
target_link_libraries(_nimble_util PRIVATE _nimble_os_abstraction_headers _nimble_host_layer)
target_include_directories(
  _nimble_util
  PUBLIC include
  PRIVATE src)
