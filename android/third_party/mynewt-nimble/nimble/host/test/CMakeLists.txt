# These tests require rootcanal HCI on port 6402
# The test currently fail.
android_add_executable(
  TARGET nimble_e2e_tests
  LICENSE Apache-2.0
  SRC src/ble_att_clt_test.c
      src/ble_att_svr_test.c
      src/ble_gap_test.c
      src/ble_gatt_conn_test.c
      src/ble_gatt_disc_c_test.c
      src/ble_gatt_disc_d_test.c
      src/ble_gatt_disc_s_test.c
      src/ble_gatt_find_s_test.c
      src/ble_gatt_read_test.c
      src/ble_gatt_write_test.c
      src/ble_gatts_notify_test.c
      src/ble_gatts_read_test.c
      src/ble_gatts_reg_test.c
      src/ble_hs_adv_test.c
      src/ble_hs_conn_test.c
      src/ble_hs_hci_test.c
      src/ble_hs_id_test.c
      src/ble_hs_pvcy_test.c
      src/ble_hs_stop_test.c
      src/ble_hs_test.c
      src/ble_hs_test_util.c
      src/ble_hs_test_util_hci.c
      src/ble_l2cap_test.c
      src/ble_os_test.c
      src/ble_sm_lgcy_test.c
      src/ble_sm_sc_test.c
      src/ble_sm_test.c
      src/ble_sm_test_util.c
      src/ble_store_test.c
      src/ble_uuid_test.c)
target_link_libraries(
  nimble_e2e_tests
  PRIVATE _nimble_test_util
          _nimble_host_layer
          _nimble_store_config
          _nimble_service_layer
          _nimble_transport_test
          _nimble_os_sysinit
          android-emu-base-headers
          android-emu-base-logging)
target_include_directories(nimble_e2e_tests PRIVATE src ../src)
target_compile_options(
  nimble_e2e_tests
  PRIVATE -Wno-implicit-function-declaration
          -Wno-tentative-definition-incomplete-type -Wno-parentheses-equality
          -Wno-incompatible-pointer-types)
