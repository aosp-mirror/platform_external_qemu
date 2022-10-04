function(add_nimble_service SVC_NAME)
  android_add_library(TARGET _nimble_${SVC_NAME} LICENSE Apache-2.0
                      SRC ${SVC_NAME}/src/ble_svc_${SVC_NAME}.c
                      DEPS android-emu-base-headers)
  target_include_directories(_nimble_${SVC_NAME} PUBLIC ${SVC_NAME}/include)
  target_link_libraries(
    _nimble_${SVC_NAME} PRIVATE _nimble_os_abstraction_headers
                                _nimble_host_layer)
endfunction()

# blueart doesn't follow pattern.
foreach(
  SVC
  ans
  bas
  dis
  gap
  gatt
  ias
  ipss
  lls
  tps)
  add_nimble_service(${SVC})
endforeach()

add_library(_nimble_service_layer INTERFACE)
target_link_libraries(
  _nimble_service_layer
  INTERFACE _nimble_ans
            _nimble_bas
            _nimble_dis
            _nimble_gap
            _nimble_gatt
            _nimble_ias
            _nimble_ipss
            _nimble_lls
            _nimble_tps)
