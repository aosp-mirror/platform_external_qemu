cmake_minimum_required(VERSION 3.5)
PREBUILT(CURL)

add_library(emulator-libkeymaster3
        aes_key.cpp
        aes_operation.cpp
        android_keymaster.cpp
        android_keymaster_messages.cpp
        android_keymaster_utils.cpp
        asymmetric_key.cpp
        asymmetric_key_factory.cpp
        attestation_record.cpp
        auth_encrypted_key_blob.cpp
        authorization_set.cpp
        ecdsa_keymaster1_operation.cpp
        ecdsa_operation.cpp
        ecies_kem.cpp
        ec_key.cpp
        ec_key_factory.cpp
        ec_keymaster0_key.cpp
        ec_keymaster1_key.cpp
        hkdf.cpp
        hmac.cpp
        hmac_key.cpp
        hmac_operation.cpp
        integrity_assured_key_blob.cpp
        iso18033kdf.cpp
        kdf.cpp
        key.cpp
        keymaster0_engine.cpp
        keymaster1_engine.cpp
        keymaster_enforcement.cpp
        keymaster_ipc.cpp
        keymaster_tags.cpp
        logger.cpp
        nist_curve_key_exchange.cpp
        ocb.c
        ocb_utils.cpp
        openssl_err.cpp
        openssl_utils.cpp
        operation.cpp
        operation_table.cpp
        rsa_key.cpp
        rsa_key_factory.cpp
        rsa_keymaster0_key.cpp
        rsa_keymaster1_key.cpp
        rsa_keymaster1_operation.cpp
        rsa_operation.cpp
        serializable.cpp
        soft_keymaster_context.cpp
        soft_keymaster_device.cpp
        symmetric_key.cpp
        trusty_keymaster_context.cpp
        trusty_keymaster_enforcement.cpp
        trusty_rng.c
        )

target_include_directories(emulator-libkeymaster3 PUBLIC .)
target_include_directories(emulator-libkeymaster3 PRIVATE src include)
target_link_libraries(emulator-libkeymaster3 PRIVATE android-emu-base OpenSSL::SSL OpenSSL::Crypto)

if ("${ANDROID_TARGET_TAG}" MATCHES "windows.*")
    # Workaround some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libkeymaster3 PRIVATE -DUSE_MINGW=1)
    endif ()
    if ("${ANDROID_TARGET_TAG}" MATCHES "windows_msvc.*")
        target_link_libraries(emulator-libkeymaster3 PRIVATE msvc-posix-compat)
        target_compile_options(emulator-libkeymaster3 PRIVATE "-mssse3")
    endif ()
endif ()

if (NOT DEFINED ANDROID_QEMU2_TOP_DIR)
  add_dependencies(emulator-libkeymaster3 boringssl)
endif ()

