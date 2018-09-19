# A static library containing the android-emu glue code

QEMU2_GLUE_INCLUDES := $(ANDROID_EMU_INCLUDES) $(EMUGL_INCLUDES)
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)/..

$(call start-cmake-project,libqemu2-glue)
LOCAL_CFLAGS += $(QEMU2_CFLAGS)

LOCAL_C_INCLUDES += \
    $(QEMU2_INCLUDES) \
    $(QEMU2_GLUE_INCLUDES) \
    $(LOCAL_PATH)/../slirp \
    $(LZ4_INCLUDES) \
    $(LIBDTB_UTILS_INCLUDES) \

PRODUCED_STATIC_LIBS=libqemu2-glue

$(call end-cmake-project)
LOCAL_PATH := $(OLD_LOCAL_PATH)

QEMU2_GLUE_STATIC_LIBRARIES := \
    libqemu2-glue \
    emulator-libui \
    emulator-lz4 \
    emulator-libdtb \
    $(EMULATOR_LIBUI_STATIC_LIBRARIES)

QEMU2_GLUE_LDFLAGS := $(EMULATOR_LIBUI_LDFLAGS)

QEMU2_GLUE_LDLIBS := $(EMULATOR_LIBUI_LDLIBS)
