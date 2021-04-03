set(LIBSRTP_ROOT ${AOSP_ROOT}/external/libsrtp2)
android_add_library(
  TARGET libsrtp
  LICENSE
    BSD-3-Clause
    URL
    https://android.googlesource.com/platform/external/libsrtp2/+/refs/heads/master
  REPO ${LIBSRTP_ROOT}
  SOURCE_DIR ${LIBSRTP_ROOT}
  NOTICE REPO/LICENSE
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

target_include_directories(
  libsrtp PUBLIC . ${LIBSRTP_ROOT}/crypto/include ${LIBSRTP_ROOT}/include)
target_link_libraries(libsrtp PUBLIC ssl)
target_compile_definitions(
  libsrtp
  PUBLIC # Package identity
         PACKAGE_STRING=\"libsrtp2_2.3.0-pre\"
         PACKAGE_VERSION=\"2.3.0-pre\"
         OPENSSL
         HAVE_CONFIG_H
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

if(NOT WINDOWS_MSVC_X86_64)
  target_compile_definitions(libsrtp PUBLIC HAVE_ARPA_INET_H HAVE_NETINET_IN_H
                                            HAVE_SYS_TYPES_H HAVE_UNISTD_H)
else()
  target_compile_definitions(libsrtp PUBLIC HAVE_WINSOCK2_H)
endif()

function(srtp_test TGT SRC PARAM)
  android_add_executable(
    TARGET ${TGT} NODISTRIBUTE SOURCE_DIR ${LIBSRTP_ROOT}
    SRC ${SRC} crypto/replay/ut_sim.c test/getopt_s.c
    test/util.c)
  target_link_libraries(${TGT} PRIVATE libsrtp Threads::Threads)
  add_test(NAME ${TGT} COMMAND $<TARGET_FILE:${TGT}> ${PARAM})
endfunction()

if(FALSE)
    # Disabled due to double symbol definitions
  srtp_test("dtls_srtp_driver" test/dtls_srtp_driver.c "")
  srtp_test("rdbx_driver" test/rdbx_driver.c "-v")
  srtp_test("replay_driver" test/replay_driver.c "")
  srtp_test("roc_driver" test/roc_driver.c "")
  srtp_test("srtp_driver" test/srtp_driver.c "-v")
  srtp_test("test_srtp" test/test_srtp.c "")
  srtp_test("cipher_driver" crypto/test/cipher_driver.c "-v")
  # srtp_test("datatypes_driver" crypto/test/datatypes_driver.c ) ??
  srtp_test("kernel_driver" crypto/test/kernel_driver.c "-v")
endif()
