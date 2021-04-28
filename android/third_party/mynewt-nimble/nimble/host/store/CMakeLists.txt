add_library(_nimble_store_config config/src/ble_store_config.c # need base64
                                 config/src/ble_store_config_conf.c)
target_link_libraries(_nimble_store_config
                      PRIVATE _nimble_os_abstraction_headers _nimble_host_layer)
target_include_directories(_nimble_store_config
                          PRIVATE config/src
                           PUBLIC config/include)

add_library(_nimble_store_ram ram/src/ble_store_ram.c
                                )
target_link_libraries(_nimble_store_ram
                      PRIVATE _nimble_os_abstraction_headers _nimble_host_layer)
target_include_directories(_nimble_store_ram
                           PUBLIC ram/include)
