# The standard test dependencies.
$(call start-emulator-library,libqemu-test)
LOCAL_C_INCLUDES += $(QEMU2_SYSTEM_INCLUDES) $(QEMU2_GLUE_INCLUDES) $(ANDROID_EMU_INCLUDES)
LOCAL_SRC_FILES += \
    tests/libqtest.c \
    $(QEMU2_AUTO_GENERATED_DIR)/tests/test-qapi-visit.c \
    $(QEMU2_AUTO_GENERATED_DIR)/tests/test-qapi-types.c \
    $(QEMU2_AUTO_GENERATED_DIR)/tests/test-qapi-events.c \
    $(QEMU2_AUTO_GENERATED_DIR)/tests/test-qapi-introspect.c \

$(call end-emulator-library)

# A variant of start-emulator-program to start the definition of a qemu test
# program instead. Use with end-emulator-program
# Tests of this type do NOT RELY on qemu/emulator binaries.
#
# A qemu test will have the postfix _qtest and will use the qemu test runner
# vs. the googletest framework.
start-emulator-qemu-test = \
    $(call start-emulator-program,$1_qtest) \
    $(eval LOCAL_STATIC_LIBRARIES += libqemu-test $(QEMU2_SYSTEM_STATIC_LIBRARIES) $(QEMU2_GLUE_STATIC_LIBRARIES) $(ANDROID_EMU_STATIC_LIBRARIES) libqemu2-util libqemu2-common) \
    $(eval LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS) $(QEMU2_GLUE_LDFLAGS) $(QEMU2_DEPS_LDFLAGS) ) \
    $(eval LOCAL_LDLIBS += $(QEMU2_SYSTEM_LDLIBS) $(QEMU2_GLIB_LDLIBS)) \
    $(eval LOCAL_CFLAGS += $(QEMU2_SYSTEM_CFLAGS) -DCONFIG_ANDROID ) \
    $(eval LOCAL_C_INCLUDES += $(QEMU2_SYSTEM_INCLUDES) $(QEMU2_GLUE_INCLUDES) $(ANDROID_EMU_INCLUDES)) \

# Adds a standard qemu test with default dependencies
add-qemu-test = \
  $(call start-emulator-qemu-test, $1) \
  $(eval LOCAL_SRC_FILES += $(addprefix tests/,$(1).c))  \
  $(call end-emulator-program) \

add-qemu-crypto-test = \
  $(call start-emulator-qemu-test, $1) \
  $(eval LOCAL_STATIC_LIBRARIES += libqemu2-common) \
  $(eval LOCAL_SRC_FILES += $(addprefix tests/,$(1).c)  \
        $(QEMU2_AUTO_GENERATED_DIR)/qapi/qapi-types-crypto.c \
        $(QEMU2_AUTO_GENERATED_DIR)/qapi/qapi-visit-crypto.c \
    )\
  $(call end-emulator-program) \

# The socket test is special, it consists of 2 files.
$(call start-emulator-qemu-test, test-util-sockets)
   LOCAL_SRC_FILES += tests/test-util-sockets.c \
                      tests/socket-helpers.c
$(call end-emulator-program)

# These tests rely on interaction with the qemu binary.
# These require a non-trivial test runner as they interact
# with a running emulator. It is not yet clear how we can run these.
TEST_NEED_QEMU := \
    test-arm-mptimer \
    test-crypto-block \
    test-bdrv-drain \
    test-block-backend \
    test-blockjob \
    test-blockjob-txn \
    test-char \
    test-filter-mirror \
    test-filter-redirector \
    test-hbitmap \
    test-hmp \
    test-io-channel-buffer \
    test-io-channel-command \
    test-io-channel-file \
    test-io-channel-socket \
    test-io-task \
    test-netfilter \
    test-qdev-global-props \
    test-qga \
    test-replication \
    test-thread-pool \
    test-throttle \
    test-vmstate \
    test-write-threshold \
    test-x86-cpuid-compat \
    test-xbzrle \
    test-crypto-tlscredsx509 \
    test-crypto-tlssession \
    test-crypto-secret \

# These require different set of linkers.
CRYPTO_TESTS := \
   test-crypto-afsplit \
   test-crypto-cipher \
   test-crypto-hash \
   test-crypto-hmac \
   test-crypto-ivgen \
   test-crypto-pbkdf \
   test-crypto-xts \
#
# These tests are currently failing.
SIMPLE_TEST_FAILURES := \
    test-qht-par \

SIMPLE_TESTS := \
    test-aio \
    test-base64 \
    test-bitcnt \
    test-bitops \
    test-bufferiszero \
    test-clone-visitor \
    test-coroutine \
    test-cutils \
    test-int128 \
    test-iov \
    test-keyval \
    test-logging \
    test-mul64 \
    test-opts-visitor \
    test-qapi-util \
    test-qdist \
    test-qemu-opts \
    test-qht \
    test-qmp-event \
    test-qobject-input-visitor \
    test-qobject-output-visitor \
    test-rcu-list \
    test-shift128 \
    test-string-input-visitor \
    test-string-output-visitor \
    test-timed-average \
    test-uuid \
    test-visitor-serialization \
    test-x86-cpuid \

# Add the lists of simple tests.
$(foreach test,$(SIMPLE_TESTS),$(call add-qemu-test, $(strip $(test))))
$(foreach test,$(CRYPTO_TESTS),$(call add-qemu-crypto-test, $(strip $(test))))
