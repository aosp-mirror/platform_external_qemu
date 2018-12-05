set(libqemu-test_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
android_add_library(libqemu-test)
target_link_libraries(libqemu-test PUBLIC android-qemu-deps)

set(libqemu-test-crypto_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
android_add_library(libqemu-test-crypto)
target_link_libraries(libqemu-test-crypto PRIVATE libqemu-test OpenSSL::Crypto)

# Adds a qemu test, note that these test should never bring in an android dependency
function(add_qtest NAME DEPENDENCY)
  set(${NAME}_src tests/${NAME}.c)
  android_add_test(${NAME})
  android_target_dependency(${NAME} linux-x86_64 TCMALLOC_OS_DEPENDENCIES)
  target_compile_definitions(${NAME} PRIVATE -DCONFIG_ANDROID)
  target_include_directories(${NAME} PUBLIC ${ANDROID_AUTOGEN}/tests)
  target_link_libraries(${NAME} PRIVATE ${DEPENDENCY} qemu2-common libqemu2-util android-qemu-deps)
endfunction()

# These tests rely on interaction with the qemu binary. These require a non- trivial test runner as they interact with a
# running emulator. It is not yet clear how we can run these.
set(TEST_NEED_QEMU
    test-arm-mptimer
    test-crypto-block
    test-bdrv-drain
    test-block-backend
    test-blockjob
    test-blockjob-txn
    test-char
    test-filter-mirror
    test-filter-redirector
    test-hbitmap
    test-hmp
    test-io-channel-buffer
    test-io-channel-command
    test-io-channel-file
    test-io-channel-socket
    test-io-task
    test-netfilter
    test-qdev-global-props
    test-qga
    test-replication
    test-thread-pool
    test-throttle
    test-vmstate
    test-write-threshold
    test-x86-cpuid-compat
    test-xbzrle
    test-crypto-tlscredsx509
    test-crypto-tlssession
    test-crypto-secret)

# These require different set of linkers.
set(CRYPTO_TESTS
    test-crypto-afsplit
    test-crypto-cipher
    test-crypto-hash
    test-crypto-hmac
    test-crypto-ivgen
    test-crypto-pbkdf
    test-crypto-xts)

# These tests are currently failing.
set(SIMPLE_TEST_FAILURES test-qht-par
    # test-rcu-list # This is a concurrency test and requires a set of parameters
    )

set(SIMPLE_TESTS
    test-aio
    test-base64
    test-bitcnt
    test-bitops
    test-bufferiszero
    test-clone-visitor
    test-coroutine
    test-cutils
    test-int128
    test-iov
    test-keyval
    # test-logging
    test-mul64
    test-opts-visitor
    test-qapi-util
    test-qdist
    test-qemu-opts
    test-qht
    test-qmp-event
    test-qobject-input-visitor
    test-qobject-output-visitor
    test-shift128
    test-string-input-visitor
    test-string-output-visitor
    test-timed-average
    test-uuid
    test-visitor-serialization
    test-x86-cpuid)

# Add basic tests
foreach(test ${SIMPLE_TESTS})
  add_qtest(${test} libqemu-test)
endforeach()

# Add crypto tests
foreach(test ${CRYPTO_TESTS})
  add_qtest(${test} libqemu-test-crypto)
endforeach()

# Socket test is split in two, so we special case it
set(test-util-sockets-qtest_src tests/test-util-sockets.c tests/socket-helpers.c)
android_add_test(test-util-sockets-qtest)
target_include_directories(test-util-sockets-qtest PUBLIC ${ANDROID_AUTOGEN}/tests)
target_link_libraries(test-util-sockets-qtest PRIVATE libqemu2-util qemu2-common android-qemu-deps)
