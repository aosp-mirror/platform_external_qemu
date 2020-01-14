set(BORINGSSL_ROOT ${ANDROID_QEMU2_TOP_DIR}/../boringssl/)
include(${BORINGSSL_ROOT}/android-sources.cmake)
enable_language(ASM)

set(ssl_src ${ssl_sources})
android_add_library(ssl)
target_include_directories(ssl PUBLIC ${BORINGSSL_ROOT}/src/include)
target_include_directories(ssl PRIVATE ${BORINGSSL_ROOT}/src/ssl)
android_target_compile_definitions(ssl windows-x86_64 PUBLIC "-DWIN32_LEAN_AND_MEAN=1")
android_target_compile_options(ssl windows PRIVATE  "-UNDEBUG")

# Setup the crypto libraries:
if(LINUX_X86_64)
  set(crypto_src ${crypto_sources} ${crypto_sources_linux_x86_64})
  android_add_library(crypto)
elseif(DARWIN_X86_64)
  set(crypto_src ${crypto_sources} ${crypto_sources_mac_x86_64})
  android_add_library(crypto)
else()
  set(crypto_src ${crypto_sources})
  android_add_library(crypto)
  android_target_compile_options(crypto windows PRIVATE "-std=c99" "-UNDEBUG")

  android_nasm_compile(TARGET crypto_asm_lib SOURCES ${crypto_sources_win_x86_64})
  target_link_libraries(crypto PRIVATE crypto_asm_lib ws2_32::ws2_32)
endif()
target_include_directories(crypto PUBLIC ${BORINGSSL_ROOT}/src/include)
target_link_libraries(ssl PUBLIC crypto)


android_license(boringssl "${BORINGSSL_ROOT}/NOTICE")
# Tests
if (NOT WINDOWS) # BUG: 147893224: clang 10 results in a flaky failure in ABITest.SanityCheck on Windows
    set(boringssl_test_support_src ${test_support_sources})
    android_add_library(boringssl_test_support)
    target_link_libraries(boringssl_test_support PRIVATE ssl)

    set(boringssl_ssl_unittests_src ${ssl_test_sources})

    android_add_test(boringssl_ssl_unittests)
    target_link_libraries(boringssl_ssl_unittests PRIVATE gmock_main ssl crypto boringssl_test_support)

    set(boringssl_crypto_unittests_src ${crypto_test_sources})

    android_add_test(boringssl_crypto_unittests)
    target_link_libraries(boringssl_crypto_unittests PRIVATE gmock_main crypto boringssl_test_support)
    android_target_link_libraries(boringssl_test_support windows PRIVATE ws2_32::ws2_32 dbghelp::dbghelp)
endif()
