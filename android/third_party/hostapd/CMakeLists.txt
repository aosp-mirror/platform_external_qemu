set(WPA_SUPPLICANT_ROOT ${ANDROID_QEMU2_TOP_DIR}/../wpa_supplicant_8/)
set(HOSTAPD_ROOT ${WPA_SUPPLICANT_ROOT}/hostapd/)

set(hostapd_src
    ${HOSTAPD_ROOT}/main.c
    ${HOSTAPD_ROOT}/config_file.c
    ${HOSTAPD_ROOT}/eap_register.c
    ${WPA_SUPPLICANT_ROOT}/src/l2_packet/l2_packet_none.c
    ${WPA_SUPPLICANT_ROOT}/src/drivers/drivers.c
    ${WPA_SUPPLICANT_ROOT}/src/drivers/driver_common.c
    ${WPA_SUPPLICANT_ROOT}/src/drivers/driver_virtio_wifi.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/common.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/wpa_debug.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/wpabuf.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/ip_addr.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/radiotap.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/uuid.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/base64.c
    ${WPA_SUPPLICANT_ROOT}/src/utils/json.c
    ${WPA_SUPPLICANT_ROOT}/src/common/ieee802_11_common.c
    ${WPA_SUPPLICANT_ROOT}/src/common/ctrl_iface_common.c
    ${WPA_SUPPLICANT_ROOT}/src/common/sae.c
    ${WPA_SUPPLICANT_ROOT}/src/common/wpa_common.c
    ${WPA_SUPPLICANT_ROOT}/src/common/hw_features_common.c
    ${WPA_SUPPLICANT_ROOT}/src/common/gas.c
    ${WPA_SUPPLICANT_ROOT}/src/common/dpp.c
    ${WPA_SUPPLICANT_ROOT}/src/eapol_auth/eapol_auth_dump.c
    ${WPA_SUPPLICANT_ROOT}/src/eapol_auth/eapol_auth_sm.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_md5.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_tls.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_peap.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/eap_peap_common.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_ttls.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_mschapv2.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_gtc.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_sim.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_aka.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/eap_sim_common.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_pwd.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/eap_pwd_common.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wps_hostapd.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_wsc.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/eap_wsc_common.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_common.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_attr_parse.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_attr_build.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_attr_process.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_dev_attr.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_enrollee.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/wps_registrar.c
    ${WPA_SUPPLICANT_ROOT}/src/wps/ndef.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/dpp_hostapd.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/gas_query_ap.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/eap_common.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_methods.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_identity.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_common/chap.c
    ${WPA_SUPPLICANT_ROOT}/src/eap_server/eap_server_tls_common.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/ms_funcs.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/tls_none.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/crypto_openssl.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/fips_prf_openssl.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/aes-siv.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/aes-ctr.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/aes-omac1.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha1-prf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha1-tlsprf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha256-prf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha256-tlsprf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha256-kdf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha384-kdf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha512-kdf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha384-prf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/sha512-prf.c
    ${WPA_SUPPLICANT_ROOT}/src/crypto/dh_groups.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/hostapd.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wnm_ap.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wpa_auth_glue.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/drv_callbacks.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ap_drv_ops.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/utils.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/authsrv.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_1x.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ap_config.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/eap_user_db.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_11_auth.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/sta_info.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wpa_auth.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/tkip_countermeasures.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ap_mlme.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ctrl_iface_ap.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wpa_auth_ie.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/preauth_auth.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/pmksa_cache_auth.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_11_shared.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/bss_load.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/neighbor_db.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/rrm.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/eth_p_oui.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wpa_auth_ft.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/wmm.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ap_list.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_11.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/hw_features.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/dfs.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_11_ht.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/ieee802_11_vht.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/hs20.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/gas_serv.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/acs.c
    ${WPA_SUPPLICANT_ROOT}/src/ap/beacon.c)

android_add_library(
  TARGET hostapd
  LIBNAME
    hostapd
    URL
    "https://android.googlesource.com/platform/external/wpa_supplicant_8/+/refs/heads/emu-master-dev"
  REPO "${WPA_SUPPLICANT_ROOT}"
  NOTICE "REPO/NOTICE"
  LICENSE "BSD"
  SRC # cmake-format: sortable
      ${hostapd_src}
  WINDOWS ${WPA_SUPPLICANT_ROOT}/src/utils/eloop_win.c
          ${WPA_SUPPLICANT_ROOT}/src/utils/os_win32.c
  LINUX ${WPA_SUPPLICANT_ROOT}/src/utils/eloop.c
        ${WPA_SUPPLICANT_ROOT}/src/utils/os_unix.c
  DARWIN ${WPA_SUPPLICANT_ROOT}/src/utils/eloop.c
         ${WPA_SUPPLICANT_ROOT}/src/utils/os_unix.c)

target_include_directories(hostapd PUBLIC ${WPA_SUPPLICANT_ROOT}/src)

target_include_directories(hostapd PRIVATE ${HOSTAPD_ROOT}
                                           ${WPA_SUPPLICANT_ROOT}/src/utils)

target_compile_definitions(
  hostapd
  PRIVATE -DWPA_IGNORE_CONFIG_ERRORS
          -DVERSION_STR_POSTFIX=\"-EMU\"
          -DHOSTAPD
          -DHOSTAPD_DUMP_STATE
          -DCONFIG_NO_RADIUS
          -DCONFIG_NO_ACCOUNTING
          -DCONFIG_NO_VLAN
          -DCONFIG_NO_CTRL_IFACE
          -DCONFIG_NO_RANDOM_POOL
          -DCONFIG_SUITEB
          -DCONFIG_SUITEB192
          -DCONFIG_ETH_P_OUI
          -DCONFIG_SAE
          -DCONFIG_WNM
          -DCONFIG_WNM_AP
          -DCONFIG_IEEE80211N
          -DCONFIG_WPS
          -DCONFIG_WPS_NFC
          -DCONFIG_WPS_OOB
          -DCONFIG_DPP
          -DPKCS12_FUNCS
          -DTLS_DEFAULT_CIPHERS="DEFAULT:!EXP:!LOW"
          -DCONFIG_SHA256
          -DCONFIG_SHA384
          -DCONFIG_SHA512
          -DCONFIG_ECC
          -DCONFIG_IPV6
          -DCONFIG_JSON
          -DNEED_AP_MLME
          -DCONFIG_HS20
          -DCONFIG_INTERWORKING
          -DCONFIG_ACS
          -DCONFIG_DRIVER_VIRTIO_WIFI)

android_target_compile_definitions(hostapd windows PRIVATE
                                   -DCONFIG_NATIVE_WINDOWS)

if(NOT LINUX_AARCH64)
  target_compile_options(
    hostapd
    PRIVATE -Wall -Werror -Wno-error-deprecated-declarations
            -Wno-unused-parameter -Wno-unused-variable -Wno-macro-redefined)
endif()

target_link_libraries(hostapd PUBLIC ssl android-emu)
