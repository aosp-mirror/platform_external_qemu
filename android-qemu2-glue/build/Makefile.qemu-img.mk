$(call start-emulator-program,qemu-img)

LOCAL_STATIC_LIBRARIES := \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \
    libqemu2-util \
    libqemu2-stubs \

LOCAL_CFLAGS := \
    $(QEMU2_SYSTEM_CFLAGS) \

LOCAL_C_INCLUDES := \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_SDL2_INCLUDES) \

LOCAL_SRC_FILES := \
    stubs/exec.c \
    qemu-img.c \
    block.c \
    blockjob.c \
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
    block/qcow2-cache.c \
    block/qcow2-cluster.c \
    block/qcow2-refcount.c \
    block/qcow2-snapshot.c \
    block/qcow2.c \
    block/qed-check.c \
    block/qed-cluster.c \
    block/qed-gencb.c \
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
    page_cache.c \
    qom/container.c \
    qom/object.c \
    qom/object_interfaces.c \
    qom/qom-qobject.c \
    replication.c \
    $(call qemu2-if-windows, \
        block/file-win32.c \
        block/win32-aio.c \
        ) \
    $(call qemu2-if-posix, \
        block/file-posix.c \
        ) \


LOCAL_LDFLAGS := $(QEMU2_SYSTEM_LDFLAGS)

LOCAL_LDLIBS := \
    $(QEMU2_SYSTEM_LDLIBS) \
    $(QEMU2_SDL2_LDLIBS) \

$(call local-link-static-c++lib)
$(call end-emulator-program)
