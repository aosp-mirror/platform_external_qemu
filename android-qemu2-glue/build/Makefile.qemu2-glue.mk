# A static library containing the android-emu glue code

QEMU2_GLUE_INCLUDES := $(ANDROID_EMU_INCLUDES)

$(call start-emulator-library,libqemu2-glue)

LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(QEMU2_GLUE_INCLUDES) \

LOCAL_SRC_FILES := \
    base/async/Looper.cpp \
    base/files/QemuFileStream.cpp \
    emulation/charpipe.c \
    emulation/CharSerialLine.cpp \
    emulation/serial_line.cpp \
    emulation/VmLock.cpp \
    looper-qemu.cpp \
    qemu-setup.cpp \
    utils/stream.cpp \

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%=android-qemu2-glue/%)

$(call end-emulator-library)
