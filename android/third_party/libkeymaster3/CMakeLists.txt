cmake_minimum_required(VERSION 3.5)

# TODO(jansene): Move to a function
if (DEFINED LOCAL_QEMU2_TOP_DIR)
    get_filename_component(REPO_ROOT "${LOCAL_QEMU2_TOP_DIR}/../.." ABSOLUTE)
    set(CURL_ROOT "${REPO_ROOT}/prebuilts/android-emulator-build/curl/${LOCAL_TARGET_TAG}")
    set(CURL_LIBRARY ${CURL_ROOT}/lib/libcurl.a)
    set(CURL_INCLUDE_DIR ${CURL_ROOT}/include)

    # We will now find the prebuilt package.
    find_package(CURL REQUIRED)
    if (NOT CURL_LIBRARIES MATCHES "${REPO_ROOT}.*")
        message(FATAL_ERROR "Discovered curl at: ${CURL_LIBRARIES}, ${CURL_INCLUDE_DIRS} which is outside of tree ${REPO_ROOT}!")
    endif ()
else ()
    message(WARNING "Building outside the make system!")
    find_package(CURL REQUIRED)

    include (ExternalProject)

    set(boringssl_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/boringssl/src/boringssl/include)
    set(boringssl_URL https://boringssl.googlesource.com/boringssl)
    set(boringssl_TAG chromium-stable)
    set(boringssl_BUILD ${CMAKE_BINARY_DIR}/boringssl/src/boringssl-build)
    #set(boringssl_LIBRARIES ${boringssl_BUILD}/obj/so/libboringssl.so)
    set(boringssl_STATIC_LIBRARIES
        ${boringssl_BUILD}/ssl/libssl.a
        ${boringssl_BUILD}/crypto/libcrypto.a
        ${boringssl_BUILD}/decrepit/libdecrepit.a
        )
    set(boringssl_INCLUDES ${boringssl_BUILD})

    set(boringssl_HEADERS
        "${boringssl_INCLUDE_DIR}/include/*.h"
        )

    ExternalProject_Add(boringssl
        PREFIX boringssl
        GIT_REPOSITORY ${boringssl_URL}
        GIT_TAG ${boringssl_TAG}
        DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
        # BUILD_IN_SOURCE 1
        BUILD_BYPRODUCTS ${boringssl_STATIC_LIBRARIES}
        INSTALL_COMMAND ""
        CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
        )

    set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIRS};${boringssl_INCLUDE_DIR}")
    set(CURL_LIBRARIES} "${CURL_LIBRARIES});${boringssl_STATIC_LIBRARIES}")
endif ()


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
target_include_directories(emulator-libkeymaster3 PUBLIC ${CURL_INCLUDE_DIRS})
target_include_directories(emulator-libkeymaster3 PRIVATE src include)
target_include_directories(emulator-libkeymaster3 PRIVATE ../../android-emu)
target_link_libraries(emulator-libkeymaster3 PUBLIC ${CURL_LIBRARIES})

if (WIN32 OR ("${LOCAL_OS_FLAVOR}" STREQUAL "windows"))
    # Workaround some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libkeymaster3 PRIVATE -DUSE_MINGW=1)
    endif ()
    if ("${LOCAL_OS}" STREQUAL "windows_msvc")
        target_include_directories(emulator-libkeymaster3 PRIVATE ../../msvc-posix-compat/include)
	target_link_libraries(emulator-libkeymaster3 PRIVATE msvc-posix-compat)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
    endif ()
endif ()

if (NOT DEFINED LOCAL_QEMU2_TOP_DIR)
  add_dependencies(emulator-libkeymaster3 boringssl)
endif ()

