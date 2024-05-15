set(BORINGSSL_ROOT ${ANDROID_QEMU2_TOP_DIR}/../boringssl/)
include(${BORINGSSL_ROOT}/android-sources.cmake)
enable_language(ASM)

set(ssl_src ${ssl_sources})
android_add_library(
  TARGET ssl
  LIBNAME
    boringssl
    URL
    "https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master"
  REPO "${BORINGSSL_ROOT}"
  NOTICE "REPO/NOTICE"
  LICENSE "OpenSSL"
  SRC # cmake-format: sortable
      ${ssl_sources})
target_include_directories(ssl PUBLIC ${BORINGSSL_ROOT}/src/include)
target_include_directories(ssl PRIVATE ${BORINGSSL_ROOT}/src/ssl)
android_target_compile_options(ssl windows PRIVATE "-UNDEBUG")

# Setup the crypto libraries: On Windows x86_64 (and x86), BoringSSL uses NASM.
# On other platforms, BoringSSL uses gas-compatible assembly syntax. Within an
# assembly syntax, the files are conditioned by the underlying platfor, so it is
# not necessary to condition on the target beyond this.
if(WINDOWS_MSVC_X86_64)
  android_add_library(
    TARGET crypto
    LIBNAME
      boringssl
      URL
      "https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master"
    REPO "${BORINGSSL_ROOT}"
    NOTICE "REPO/NOTICE"
    LICENSE "OpenSSL"
    SRC # cmake-format: sortable
        ${crypto_sources})

  android_target_compile_options(crypto windows PRIVATE "-std=c11" "-UNDEBUG")

  android_nasm_compile(
    TARGET crypto_asm_lib
    LIBNAME
      boringssl
      URL
      "https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master"
    REPO "${BORINGSSL_ROOT}"
    NOTICE "REPO/NOTICE"
    LICENSE "OpenSSL"
    SRC # cmake-format: sortable
        ${crypto_sources_nasm})
  target_link_libraries(crypto PRIVATE crypto_asm_lib ws2_32::ws2_32)
else()
  android_add_library(
    TARGET crypto
    LIBNAME
      boringssl
      URL
      "https://android.googlesource.com/platform/external/boringssl/+/refs/heads/master"
    REPO "${BORINGSSL_ROOT}"
    NOTICE "REPO/NOTICE"
    LICENSE "OpenSSL"
    SRC # cmake-format: sortable
        ${crypto_sources_asm} ${crypto_sources})
endif()
target_include_directories(crypto PUBLIC ${BORINGSSL_ROOT}/src/include)
target_link_libraries(ssl PUBLIC crypto)

# Tests
if((NOT WINDOWS) AND (NOT DARWIN_AARCH64))
  # BUG: 147893224: clang 10 results in a flaky failure in ABITest.SanityCheck
  # on Windows BUG: 252237763: timeout on Mac M2
  android_add_library(
    TARGET boringssl_test_support NODISTRIBUTE SRC # cmake-format: sortable
                                                   ${test_support_sources})
  target_link_libraries(boringssl_test_support PRIVATE ssl gmock_main)
  android_add_test(TARGET boringssl_ssl_unittests SRC # cmake-format: sortable
                                                      ${ssl_test_sources})
  target_link_libraries(boringssl_ssl_unittests PRIVATE gmock_main ssl crypto
                                                        boringssl_test_support)
  set_property(
    TEST boringssl_ssl_unittests
    PROPERTY ENVIRONMENT "BORINGSSL_TEST_DATA_ROOT=${BORINGSSL_ROOT}/src")
  android_add_test(
    TARGET boringssl_crypto_unittests SRC # cmake-format: sortable
                                          ${crypto_test_sources})
  target_link_libraries(boringssl_crypto_unittests
                        PRIVATE gmock_main crypto boringssl_test_support)
  set_property(
    TEST boringssl_crypto_unittests
    PROPERTY ENVIRONMENT "BORINGSSL_TEST_DATA_ROOT=${BORINGSSL_ROOT}/src")
  android_target_link_libraries(boringssl_test_support windows
                                PRIVATE ws2_32::ws2_32 dbghelp::dbghelp)
endif()
