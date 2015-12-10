# A static library containing the android-emu glue code

$(call start-emulator-library,libqemu2_glue)

LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(LOCAL_PATH)/slirp \
    $(LOCAL_PATH)/tcg \

LOCAL_SRC_FILES := \
    android_qemud.cpp \
    display.cpp \
    looper-qemu.cpp \
    net-android.cpp \
    qemu-battery-agent-impl.c \
    qemu-cellular-agent-impl.c \
    qemu-display-agent-impl.cpp \
    qemu-finger-agent-impl.c \
    qemu-location-agent-impl.c \
    qemu-net-agent-impl.c \
    qemu-sensors-agent-impl.c \
    qemu-setup.cpp \
    qemu-telephony-agent-impl.c \
    qemu-user-event-agent-impl.c \
    qemu-vm-operations-impl.c \
    qemu-window-agent-impl.c \
    utils/stream.cpp \
    base/async/Looper.cpp \
    base/files/QemuFileStream.cpp \
    emulation/charpipe.c \
    emulation/CharSerialLine.cpp \
    emulation/serial_line.cpp \
    telephony/modem_init.c \

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%=android-qemu2-glue/%)

$(call end-emulator-library)

QEMU2_GLUE_STATIC_LIBRARIES := libqemu2_glue
