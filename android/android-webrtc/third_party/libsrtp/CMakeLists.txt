android_add_library(
  TARGET libsrtp
  LICENSE
    BSD-3-Clause
    URL
    https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev
  REPO ${ANDROID_QEMU2_TOP_DIR}
  NOTICE REPO/android/android-webrtc/third_party/libsrtp/LICENSE
  SRC # cmake-format: sortable sources
      crypto/cipher/aes_gcm_ossl.c
      crypto/cipher/aes_icm_ossl.c
      crypto/cipher/cipher.c
      crypto/cipher/null_cipher.c
      crypto/hash/auth.c
      crypto/hash/hmac_ossl.c
      crypto/hash/null_auth.c
      crypto/kernel/alloc.c
      crypto/kernel/crypto_kernel.c
      crypto/kernel/err.c
      crypto/kernel/key.c
      crypto/math/datatypes.c
      crypto/math/stat.c
      crypto/replay/rdb.c
      crypto/replay/rdbx.c
      srtp/ekt.c
      srtp/srtp.c)

target_include_directories(libsrtp PUBLIC crypto/include include config)
target_link_libraries(libsrtp PUBLIC ssl)
target_compile_definitions(
  libsrtp
  PUBLIC # Package identity
         PACKAGE_STRING=\"860492290f7d1f25e2bd45da6471bfd4cd4d7add\"
         PACKAGE_VERSION=\"860492290f7d1f25e2bd45da6471bfd4cd4d7add\"
         # Config
         HAVE_CONFIG_H
         OPENSSL
         GCM
         # Platform properties
         HAVE_STDLIB_H
         HAVE_STRING_H
         HAVE_STDINT_H
         HAVE_INTTYPES_H
         HAVE_INT16_T
         HAVE_INT32_T
         HAVE_INT8_T
         HAVE_UINT16_T
         HAVE_UINT32_T
         HAVE_UINT64_T
         HAVE_UINT8_T)

android_target_compile_definitions(
  libsrtp darwin-x86_64 PUBLIC HAVE_ARPA_INET_H HAVE_NETINET_IN_H
                               HAVE_SYS_TYPES_H HAVE_UNISTD_H)

android_target_compile_definitions(
  libsrtp linux-x86_64 PUBLIC HAVE_ARPA_INET_H HAVE_NETINET_IN_H
                              HAVE_SYS_TYPES_H HAVE_UNISTD_H)

android_target_compile_definitions(libsrtp windows_msvc-x86_64
                                   PRIVATE HAVE_WINSOCK2_H)

function(srtp_test TGT SRC PARAM)
  add_executable(${TGT} ${SRC} crypto/replay/ut_sim.c test/getopt_s.c
                        test/util.c)
  target_link_libraries(${TGT} PRIVATE libsrtp)
  add_test(NAME ${TGT} COMMAND $<TARGET_FILE:${TGT}> ${PARAM})
endfunction()

srtp_test("dtls_srtp_driver" test/dtls_srtp_driver.c "")
srtp_test("rdbx_driver" test/rdbx_driver.c "-v")
srtp_test("replay_driver" test/replay_driver.c "")
srtp_test("roc_driver" test/roc_driver.c "")
srtp_test("srtp_driver" test/srtp_driver.c "-v")
srtp_test("test_srtp" test/test_srtp.c "")
srtp_test("cipher_driver" crypto/test/cipher_driver.c "-v")
# srtp_test("datatypes_driver" crypto/test/datatypes_driver.c ) ??
srtp_test("kernel_driver" crypto/test/kernel_driver.c "-v")
