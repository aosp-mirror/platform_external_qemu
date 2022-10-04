android_add_library(TARGET _nimble_util LICENSE Apache-2.0 SRC src/addr.c)
target_link_libraries(_nimble_util PRIVATE _nimble_os_abstraction_headers
                                           _nimble_host_layer
                                           android-emu-base-headers)
target_include_directories(_nimble_util PUBLIC include PRIVATE src)
