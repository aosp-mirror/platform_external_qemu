if(ENABLE_MESH)
  android_add_library(
    TARGET
    LICENSE Apache-2.0
    SRC mesh/src/access.c
        mesh/src/adv.c
        mesh/src/aes-ccm.c
        mesh/src/app_keys.c
        mesh/src/beacon.c
        mesh/src/cdb.c
        mesh/src/cfg.c
        mesh/src/cfg_cli.c
        mesh/src/cfg_srv.c
        mesh/src/crypto.c
        mesh/src/friend.c
        mesh/src/glue.c
        mesh/src/health_cli.c
        mesh/src/health_srv.c
        # mesh/src/testing.c
        mesh/src/heartbeat.c
        mesh/src/light_model.c
        mesh/src/lpn.c
        mesh/src/mesh.c
        mesh/src/model_cli.c
        mesh/src/model_srv.c
        mesh/src/net.c
        mesh/src/pb_adv.c
        mesh/src/pb_gatt.c
        mesh/src/prov.c
        mesh/src/prov_device.c
        mesh/src/provisioner.c
        mesh/src/proxy.c
        mesh/src/rpl.c
        mesh/src/settings.c
        mesh/src/shell.c
        mesh/src/subnet.c
        mesh/src/transport.c
        nimble_mesh
    DEPS android-emu-base-headers)
  target_link_libraries(nimble_mesh PRIVATE _nimble_os_abstraction_headers
                                            _nimble_tinycrypt)
  target_include_directories(nimble_mesh PUBLIC include mesh/include
                             PRIVATE mesh/src)
endif()

android_add_library(
  TARGET _nimble_host_layer
  LICENSE Apache-2.0
  SRC src/ble_att.c
      src/ble_att_clt.c
      src/ble_att_cmd.c
      src/ble_att_svr.c
      src/ble_eddystone.c
      src/ble_gap.c
      src/ble_gattc.c
      src/ble_gatts.c
      src/ble_hs.c
      src/ble_hs_adv.c
      src/ble_hs_atomic.c
      src/ble_hs_cfg.c
      src/ble_hs_conn.c
      src/ble_hs_flow.c
      src/ble_hs_hci.c
      src/ble_hs_hci_cmd.c
      src/ble_hs_hci_evt.c
      src/ble_hs_hci_util.c
      src/ble_hs_id.c
      src/ble_hs_log.c
      src/ble_hs_mbuf.c
      src/ble_hs_misc.c
      src/ble_hs_mqueue.c
      src/ble_hs_periodic_sync.c
      src/ble_hs_pvcy.c
      src/ble_hs_shutdown.c
      src/ble_hs_startup.c
      src/ble_hs_stop.c
      src/ble_ibeacon.c
      src/ble_l2cap.c
      src/ble_l2cap_coc.c
      src/ble_l2cap_sig.c
      src/ble_l2cap_sig_cmd.c
      src/ble_monitor.c
      src/ble_sm.c
      src/ble_sm_alg.c
      src/ble_sm_cmd.c
      src/ble_sm_lgcy.c
      src/ble_sm_sc.c
      src/ble_store.c
      src/ble_store_util.c
      src/ble_uuid.c
      # src/ble_gatts_lcl.c # needs console
      DEPS android-emu-base-headers
)
target_link_libraries(
  _nimble_host_layer
  PRIVATE _nimble_os_abstraction_headers _nimble_os_abstraction _nimble_os_impl
          _nimble_tinycrypt PUBLIC android-emu-base-logging)
target_include_directories(_nimble_host_layer PUBLIC include PRIVATE .)
target_compile_options(
  _nimble_host_layer PRIVATE -Wno-implicit-function-declaration
                             -Wno-parentheses-equality)

add_subdirectory(util)
add_subdirectory(store)
add_subdirectory(services)
