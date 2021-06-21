android_add_library(
  TARGET nimble_controller
  LICENSE Apache-2.0
  SRC src/ble_ll.c
      src/ble_ll_adv.c
      src/ble_ll_conn.c
      src/ble_ll_conn_hci.c
      src/ble_ll_ctrl.c
      src/ble_ll_dtm.c
      src/ble_ll_hci.c
      src/ble_ll_hci_ev.c
      src/ble_ll_iso.c
      src/ble_ll_rand.c
      src/ble_ll_resolv.c
      src/ble_ll_rfmgmt.c
      src/ble_ll_scan.c
      src/ble_ll_sched.c
      src/ble_ll_supp_cmd.c
      src/ble_ll_sync.c
      src/ble_ll_trace.c
      src/ble_ll_utils.c
      src/ble_ll_whitelist.c)
target_include_directories(nimble_controller PUBLIC include)
target_link_libraries(nimble_controller PUBLIC _nimble_os_abstraction_headers
                                               nimble_native_driver)
