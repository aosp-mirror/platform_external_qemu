# Rules to build the final target-specific QEMU2 binaries from sources.

# The QEMU-specific target name for a given emulator CPU architecture.
QEMU2_TARGET_CPU_x86 := i386
QEMU2_TARGET_CPU_x86_64 := x86_64
QEMU2_TARGET_CPU_mips := mipsel
QEMU2_TARGET_CPU_mips64 := mips64el
QEMU2_TARGET_CPU_arm := arm
QEMU2_TARGET_CPU_arm64 := aarch64

# The engine will use the sources from target-<name> to emulate a given
# emulator CPU architecture.
QEMU2_TARGET_TARGET_x86 := i386
QEMU2_TARGET_TARGET_x86_64 := i386
QEMU2_TARGET_TARGET_mips := mips
QEMU2_TARGET_TARGET_mips64 := mips
QEMU2_TARGET_TARGET_arm := arm
QEMU2_TARGET_TARGET_arm64 := arm

# As a special case, the 32-bit ARM binary is named qemu-system-armel to match
QEMU2_TARGET_CPU := $(QEMU2_TARGET_CPU_$(QEMU2_TARGET))
QEMU2_TARGET_TARGET := $(QEMU2_TARGET_TARGET_$(QEMU2_TARGET))

# upstream QEMU, even though the upstream configure script now uses the 'arm'
# target name (and stores the build files under arm-softmmu instead of
# armel-softmmu. qemu-system-armel is also  what the top-level 'emulator'
# launcher expects.
QEMU2_TARGET_SYSTEM := $(QEMU2_TARGET_CPU)
ifeq (arm,$(QEMU2_TARGET))
    QEMU2_TARGET_SYSTEM := armel
endif

# If $(QEMU2_TARGET) is $1, then return $2, or the empty string otherwise.
qemu2-if-target = $(if $(filter $1,$(QEMU2_TARGET)),$2,$3)

# If $(QEMU2_TARGET_CPU) is $1, then return $2, or the empty string otherwise.
qemu2-if-target-arch = $(if $(filter $1,$(QEMU2_TARGET_CPU)),$2)

# A library that contains all QEMU2 target-specific sources, excluding
# anything implemented by the glue code.

QEMU2_SYSTEM_CFLAGS := \
    $(QEMU2_CFLAGS) \
    -DNEED_CPU_H \

QEMU2_SYSTEM_INCLUDES := \
    $(QEMU2_INCLUDES) \
    $(QEMU2_DEPS_TOP_DIR)/include \
    $(call qemu2-if-linux,$(LOCAL_PATH)/linux-headers) \
    $(LOCAL_PATH)/android-qemu2-glue/config/target-$(QEMU2_TARGET) \
    $(LOCAL_PATH)/target/$(QEMU2_TARGET_TARGET) \
    $(LOCAL_PATH)/tcg \
    $(LOCAL_PATH)/tcg/i386 \

QEMU2_SYSTEM_LDFLAGS := $(QEMU2_DEPS_LDFLAGS)

QEMU2_SYSTEM_LDLIBS := \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    $(call qemu2-if-windows, -lvfw32 -ldxguid) \
    $(call qemu2-if-linux, -lpulse) \
    $(call qemu2-if-darwin, -weak_framework Hypervisor) \

QEMU2_SYSTEM_STATIC_LIBRARIES := \
    emulator-zlib \
    $(LIBUSB_STATIC_LIBRARIES) \
    $(call qemu2-if-linux, emulator-libbluez) \



$(call start-emulator-library,libqemu2-system-$(QEMU2_TARGET_SYSTEM))

LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \
    -DPOISON_CONFIG_ANDROID \

LOCAL_C_INCLUDES += \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(call qemu2-if-target,arm arm64,$(LOCAL_PATH)/disas/libvixl) \

LOCAL_SRC_FILES += \
    $(QEMU2_TARGET_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES) \
    $(QEMU2_TARGET_$(QEMU2_TARGET_CPU)_SOURCES_$(BUILD_TARGET_TAG))

ifeq (arm64,$(QEMU2_TARGET))
LOCAL_GENERATED_SOURCES += $(QEMU2_AUTO_GENERATED_DIR)/gdbstub-xml-arm64.c
else ifeq (arm,$(QEMU2_TARGET))
LOCAL_GENERATED_SOURCES += $(QEMU2_AUTO_GENERATED_DIR)/gdbstub-xml-arm.c
else
LOCAL_SRC_FILES += stubs/gdbstub.c
endif

LOCAL_SRC_FILES += \
    stubs/arch-query-cpu-model-baseline.c \
    stubs/arch-query-cpu-model-comparison.c \
    stubs/target-get-monitor-def.c \
    $(call qemu2-if-target,arm arm64 mips mips64,\
		stubs/arch-query-cpu-model-expansion.c \
        stubs/qmp_pc_dimm_device_list.c \
		stubs/vmgenid.c \
        ) \
    $(call qemu2-if-target,x86 x86_64,, \
        stubs/pc_madt_cpu_entry.c \
        hw/smbios/smbios_type_38-stub.c \
        stubs/target-monitor-defs.c \
        ) \
    $(call qemu2-if-target,mips mips64, \
        stubs/dump.c \
        stubs/arch-query-cpu-def.c \
        ) \

LOCAL_SRC_FILES += \
    $(call qemu2-if-target,x86 x86_64, \
        $(call qemu2-if-linux, hax-stub.c), \
        hax-stub.c \
    ) \
    $(call qemu2-if-target,x86 x86_64, \
        $(call qemu2-if-os, linux darwin, whpx-stub.c), \
        whpx-stub.c \
    ) \
    $(call qemu2-if-target,x86 x86_64, \
        $(call qemu2-if-os, linux windows, hvf-stub.c), \
        hvf-stub.c \
    ) \


LOCAL_PREBUILTS_OBJ_FILES += \
    $(call qemu2-if-windows,$(QEMU2_AUTO_GENERATED_DIR)/version.o)

$(call end-emulator-library)

ifeq (,$(CONFIG_MIN_BUILD))
    # The upstream version of QEMU2, without any Android-specific hacks.
    # This uses the regular SDL2 UI backend.

    $(call start-emulator-program,qemu-upstream-$(QEMU2_TARGET_SYSTEM))

    LOCAL_WHOLE_STATIC_LIBRARIES += \
        libqemu2-system-$(QEMU2_TARGET_SYSTEM) \
        libqemu2-common \

    LOCAL_STATIC_LIBRARIES += \
        $(QEMU2_SYSTEM_STATIC_LIBRARIES) \

    LOCAL_CFLAGS += \
        $(QEMU2_SYSTEM_CFLAGS) \

    LOCAL_C_INCLUDES += \
        $(QEMU2_SYSTEM_INCLUDES) \
        $(QEMU2_SDL2_INCLUDES) \

    LOCAL_SRC_FILES += \
        $(call qemu2-if-target,x86 x86_64, \
            hw/i386/acpi-build.c \
            hw/i386/pc_piix.c \
            ) \
        $(call qemu2-if-windows, \
            stubs/win32-stubs.c \
            ) \
        ui/sdl2.c \
        ui/sdl2-input.c \
        ui/sdl2-2d.c \
        vl.c \

    LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS)

    LOCAL_LDLIBS += \
        $(QEMU2_SYSTEM_LDLIBS) \
        $(QEMU2_SDL2_LDLIBS) \

    LOCAL_INSTALL_DIR := qemu/$(BUILD_TARGET_TAG)

    $(call end-emulator-program)
endif   # !CONFIG_MIN_BUILD

# The emulator-specific version of QEMU2, with CONFIG_ANDROID defined at
# compile-time.

$(call start-emulator-program,qemu-system-$(QEMU2_TARGET_SYSTEM))

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2-system-$(QEMU2_TARGET_SYSTEM) \
    libqemu2-common \

LOCAL_STATIC_LIBRARIES += \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \
    -DCONFIG_ANDROID \

LOCAL_C_INCLUDES += \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_GLUE_INCLUDES) \
    $(ANDROID_EMU_INCLUDES) \

# For now, use stubs/sdl-null.c as an empty/fake SDL UI backend.
# TODO: Use the glue code to use the Qt-based UI instead.
LOCAL_SRC_FILES += \
    android-qemu2-glue/main.cpp \
    $(call qemu2-if-target,x86 x86_64, \
        hw/i386/acpi-build.c \
        hw/i386/pc_piix.c \
        ) \
    $(call qemu2-if-windows, \
        android-qemu2-glue/stubs/win32-stubs.c \
        ) \
    vl.c \

LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS) $(QEMU2_GLUE_LDFLAGS)
LOCAL_LDLIBS += $(QEMU2_SYSTEM_LDLIBS) $(QEMU2_GLUE_LDLIBS)

LOCAL_LDFLAGS += \
    $(QEMU2_DEPS_LDFLAGS) \

LOCAL_LDLIBS += \
    $(QEMU2_GLIB_LDLIBS) \
    $(QEMU2_PIXMAN_LDLIBS) \
    $(CXX_STD_LIB) \
    -lfdt \
    $(call qemu2-if-windows, -lvfw32 -ldxguid) \
    $(call qemu2-if-linux, -lpulse) \
    $(ANDROID_EMU_LDLIBS) \

LOCAL_INSTALL_DIR := qemu/$(BUILD_TARGET_TAG)

$(call end-emulator-program)

# qemu-img

$(call start-emulator-program,qemu-img)

LOCAL_STATIC_LIBRARIES := \
    $(QEMU2_SYSTEM_STATIC_LIBRARIES) \

LOCAL_CFLAGS := \
    $(QEMU2_SYSTEM_CFLAGS) \

LOCAL_C_INCLUDES := \
    $(QEMU2_SYSTEM_INCLUDES) \
    $(QEMU2_SDL2_INCLUDES) \

LOCAL_SRC_FILES += \
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
    crypto/pbkdf-stub.c \
    qapi/opts-visitor.c \
    qapi/qapi-clone-visitor.c \
    qapi/qapi-dealloc-visitor.c \
    qapi/qapi-util.c \
    qapi/qapi-visit-core.c \
    qapi/qobject-input-visitor.c \
    qapi/qobject-output-visitor.c \
    qapi/qmp-dispatch.c \
    qapi/qmp-event.c \
    qapi/qmp-registry.c \
    qapi/string-input-visitor.c \
    qapi/string-output-visitor.c \
    qobject/json-lexer.c \
    qobject/json-parser.c \
    qobject/json-streamer.c \
    qobject/qbool.c \
    qobject/qdict.c \
    qobject/qfloat.c \
    qobject/qint.c \
    qobject/qjson.c \
    qobject/qlist.c \
    qobject/qnull.c \
    qobject/qobject.c \
    qobject/qstring.c \
    stubs/arch-query-cpu-def.c \
    stubs/arch-query-cpu-model-baseline.c \
    stubs/arch-query-cpu-model-comparison.c \
    stubs/arch-query-cpu-model-expansion.c \
    stubs/bdrv-next-monitor-owned.c \
    stubs/blockdev-close-all-bdrv-states.c \
    stubs/clock-warp.c \
    stubs/cpu-get-clock.c \
    stubs/cpu-get-icount.c \
    stubs/dump.c \
    stubs/error-printf.c \
    stubs/exec.c \
    stubs/fdset.c \
    stubs/gdbstub.c \
    stubs/get-vm-name.c \
    stubs/iothread-lock.c \
    stubs/is-daemonized.c \
    stubs/machine-init-done.c \
    stubs/migr-blocker.c \
    stubs/migration.c \
    stubs/monitor.c \
    stubs/pc_madt_cpu_entry.c \
    stubs/qmp_pc_dimm_device_list.c \
    stubs/qtest.c \
    stubs/replay.c \
    stubs/runstate-check.c \
    stubs/slirp.c \
    stubs/sysbus.c \
    stubs/target-get-monitor-def.c \
    stubs/target-monitor-defs.c \
    stubs/trace-control.c \
    stubs/uuid.c \
    stubs/vm-stop.c \
    stubs/vmgenid.c \
    stubs/vmstate.c \
    trace/control.c \
    trace/qmp.c \
    util/abort.c \
    util/acl.c \
    util/aiocb.c \
    util/async.c \
    util/base64.c \
    util/bitmap.c \
    util/bitops.c \
    util/buffer.c \
    util/bufferiszero.c \
    util/crc32c.c \
    util/cutils.c \
    util/envlist.c \
    util/error.c \
    util/getauxval.c \
    util/hexdump.c \
    util/hbitmap.c \
    util/id.c \
    util/iohandler.c \
    util/iov.c \
    util/keyval.c \
    util/lockcnt.c \
    util/log.c \
    util/main-loop.c \
    util/module.c \
    util/notify.c \
    util/osdep.c \
    util/path.c \
    util/qdist.c \
    util/qemu-config.c \
    util/qemu-coroutine.c \
    util/qemu-coroutine-io.c \
    util/qemu-coroutine-lock.c \
    util/qemu-coroutine-sleep.c \
    util/qemu-error.c \
    util/qemu-option.c \
    util/qemu-progress.c \
    util/qemu-sockets.c \
    util/qemu-timer.c \
    util/qemu-timer-common.c \
    util/qht.c \
    util/range.c \
    util/rcu.c \
    util/readline.c \
    util/timed-average.c \
    util/thread-pool.c \
    util/throttle.c \
    util/unicode.c \
    util/uri.c \
    util/uuid.c \
    $(call qemu2-if-windows, \
        block/file-win32.c \
        block/win32-aio.c \
        stubs/win32-stubs.c \
        util/aio-win32.c \
        util/coroutine-win32.c \
        util/event_notifier-win32.c \
        util/oslib-win32.c \
        util/qemu-thread-win32.c \
        ) \
    $(call qemu2-if-linux, \
        util/coroutine-ucontext.c \
        util/memfd.c \
        ) \
    $(call qemu2-if-darwin, \
        util/coroutine-sigaltstack.c \
        ) \
    $(call qemu2-if-posix, \
        block/file-posix.c \
        stubs/fd-register.c \
        util/aio-posix.c \
        util/event_notifier-posix.c \
        util/mmap-alloc.c \
        util/oslib-posix.c \
        util/qemu-openpty.c \
        util/qemu-thread-posix.c \
        ) \
    $(call qemu2-if-build-target-arch,x86, util/host-utils.c) \
    $(call qemu2-if-posix, \
        util/compatfd.c \
        ) \

LOCAL_GENERATED_SOURCES += \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-event.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-types.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qapi-visit.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qmp-introspect.c \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-tcg-tracers.h \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-helpers-wrappers.h \
    $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-helpers.h \
    $(QEMU2_AUTO_GENERATED_DIR)/trace-root.c \
    $(QEMU2_AUTO_GENERATED_DIR)/util/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/crypto/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/io/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/migration/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/block/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/backends/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/block/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/block/dataplane/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/char/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/intc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/net/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/virtio/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/audio/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/misc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/usb/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/scsi/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/nvram/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/display/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/input/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/timer/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/dma/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/sparc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/sd/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/isa/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/mem/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/i386/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/i386/xen/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/9pfs/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/ppc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/pci/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/s390x/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/vfio/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/acpi/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/arm/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/alpha/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/hw/xen/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/ui/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/audio/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/net/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/arm/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/i386/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/mips/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/sparc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/s390x/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/target/ppc/trace.c \
    $(QEMU2_AUTO_GENERATED_DIR)/qom/trace.c \
    # $(QEMU2_AUTO_GENERATED_DIR)/trace/generated-helpers.c \

LOCAL_LDFLAGS := $(QEMU2_SYSTEM_LDFLAGS)

LOCAL_LDLIBS := \
    $(QEMU2_SYSTEM_LDLIBS) \
    $(QEMU2_SDL2_LDLIBS) \

$(call local-link-static-c++lib)
$(call end-emulator-program)
