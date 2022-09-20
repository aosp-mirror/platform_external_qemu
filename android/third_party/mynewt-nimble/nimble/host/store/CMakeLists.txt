android_add_library(
  TARGET _nimble_store_config LICENSE Apache-2.0
  SRC config/src/ble_store_config.c # need base64
      config/src/ble_store_config_conf.c
  DEPS android-emu-base-headers)
target_link_libraries(_nimble_store_config
                      PRIVATE _nimble_os_abstraction_headers _nimble_host_layer)
target_include_directories(_nimble_store_config PRIVATE config/src
                           PUBLIC config/include)

android_add_library(TARGET _nimble_store_ram LICENSE Apache-2.0
                    SRC ram/src/ble_store_ram.c
                    DEPS android-emu-base-headers)
target_link_libraries(
  _nimble_store_ram PRIVATE _nimble_os_abstraction_headers _nimble_host_layer
                            android-emu-base-logging)
target_include_directories(_nimble_store_ram PUBLIC ram/include)
