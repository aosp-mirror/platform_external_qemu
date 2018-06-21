$(call start-emulator-program,qemu-img)

LOCAL_STATIC_LIBRARIES := \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \
    libqemu2-util \

LOCAL_CFLAGS := \
    $(QEMU2_SYSTEM_CFLAGS) \

LOCAL_C_INCLUDES := \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_SDL2_INCLUDES) \

LOCAL_SRC_FILES := \
    block.c \
    block/accounting.c \
    block/backup.c \
    block/blkdebug.c \
    block/blkreplay.c \
    block/blkverify.c \
    block/block-backend.c \
    block/bochs.c \
    block/cat.c \
    block/cloop.c \
    block/commit.c \
    block/crypto.c \
    block/dirty-bitmap.c \
    block/dmg.c \
    block/io.c \
    block/mirror.c \
    block/nbd-client.c \
    block/nbd.c \
    block/null.c \
    block/parallels.c \
    block/qapi.c \
    block/qcow.c \
    block/qcow2-bitmap.c \
    block/qcow2-cache.c \
    block/qcow2-cluster.c \
    block/qcow2-refcount.c \
    block/qcow2-snapshot.c \
    block/qcow2.c \
    block/qed-check.c \
    block/qed-cluster.c \
    block/qed-l2-cache.c \
    block/qed-table.c \
    block/qed.c \
    block/quorum.c \
    block/raw-format.c \
    block/replication.c \
    block/sheepdog.c \
    block/snapshot.c \
    block/stream.c \
    block/throttle-groups.c \
    block/vdi.c \
    block/vhdx-endian.c \
    block/vhdx-log.c \
    block/vhdx.c \
    block/vmdk.c \
    block/vpc.c \
    block/vvfat.c \
    block/write-threshold.c \
    blockdev-nbd.c \
    blockjob.c \
    crypto/aes.c \
    crypto/afsplit.c \
    crypto/block-luks.c \
    crypto/block-qcow.c \
    crypto/block.c \
    crypto/cipher.c \
    crypto/desrfb.c \
    crypto/hash-glib.c \
    crypto/hash.c \
    crypto/hmac-glib.c \
    crypto/hmac.c \
    crypto/init.c \
    crypto/ivgen-essiv.c \
    crypto/ivgen-plain.c \
    crypto/ivgen-plain64.c \
    crypto/ivgen.c \
    crypto/pbkdf.c \
    crypto/random-platform.c \
    crypto/secret.c \
    crypto/tlscreds.c \
    crypto/tlscredsanon.c \
    crypto/tlscredsx509.c \
    crypto/tlssession.c \
    crypto/xts.c \
    disas/i386.c \
    io/channel-buffer.c \
    io/dns-resolver.c \
    io/net-listener.c \
    io/channel-command.c \
    io/channel-file.c \
    io/channel-socket.c \
    io/channel-tls.c \
    io/channel-util.c \
    io/channel-watch.c \
    io/channel-websock.c \
    io/channel.c \
    io/task.c \
    iothread.c \
    nbd/client.c \
    nbd/common.c \
    nbd/server.c \
    qemu-img.c \
    qom/container.c \
    qom/object.c \
    qom/object_interfaces.c \
    qom/qom-qobject.c \
    replication.c \
    stubs/exec.c \
    $(QEMU2_AUTO_GENERATED_DIR)/block/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/crypto/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/io/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qom/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace-root.c \
    $(QEMU2_AUTO_GENERATED_DIR)/util/trace.c \
    $(call qemu2-if-windows, \
        block/file-win32.c \
        block/win32-aio.c \
        ) \
    $(call qemu2-if-linux, \
        scsi/pr-manager.c \
        )\
    $(call qemu2-if-posix, \
        block/file-posix.c \
        ) \


LOCAL_LDFLAGS := $(QEMU2_SYSTEM_LDFLAGS)

LOCAL_LDLIBS := \
    $(QEMU2_SYSTEM_LDLIBS) \
    $(QEMU2_SDL2_LDLIBS) \

$(call local-link-static-c++lib)
$(call end-emulator-program)
