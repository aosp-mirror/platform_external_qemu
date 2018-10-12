
set(libqemu-test_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
set(libqemu-test_libs_public android-qemu-deps)
add_android_library(libqemu-test)

set(libqemu-test-crypto_src
    tests/libqtest.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-visit.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-types.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-events.c
    ${ANDROID_AUTOGEN}/tests/test-qapi-introspect.c)
set(libqemu-test-crypto_libs_public libqemu-test ${CURL_LIBRARIES})
add_android_library(libqemu-test-crypto)

# Adds a qemu test, note that these test should never bring in an android
# dependency
function(add_qtest NAME DEPENDENCY)
  set(${NAME}_src tests/${NAME}.c)
  set(${NAME}_includes_public ${ANDROID_AUTOGEN}/tests)
  set(${NAME}_libs_private
      ${DEPENDENCY}
      libqemu2-common
      libqemu2-util
      android-qemu-deps)
  add_android_test(${NAME})
endfunction()

# These tests rely on interaction with the qemu binary. These require a non-
# trivial test runner as they interact with a running emulator. It is not yet
# clear how we can run these.
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
set(SIMPLE_TEST_FAILURES test-qht-par)

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
    test-logging
    test-mul64
    test-opts-visitor
    test-qapi-util
    test-qdist
    test-qemu-opts
    test-qht
    test-qmp-event
    test-qobject-input-visitor
    test-qobject-output-visitor
    test-rcu-list
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
set(test-util-sockets-qtest_src tests/test-util-sockets.c
    tests/socket-helpers.c)
set(test-util-sockets-qtest_includes_public ${ANDROID_AUTOGEN}/tests)
set(test-util-sockets-qtest_libs_private
    libqemu2-util
    libqemu2-common
    android-qemu-deps)
add_android_test(test-util-sockets-qtest)

