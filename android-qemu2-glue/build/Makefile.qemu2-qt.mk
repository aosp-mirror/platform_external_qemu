##############################################################################

ANDROID_SKIN_LDLIBS :=
ANDROID_SKIN_LDFLAGS :=
ANDROID_SKIN_CFLAGS :=
ANDROID_SKIN_QT_MOC_SRC_FILES :=
ANDROID_SKIN_QT_RESOURCES :=
ANDROID_SKIN_SOURCES :=
ANDROID_SKIN_STATIC_LIBRARIES :=

##############################################################################
# start with the skin sources

QT_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(QEMU2_TOP_DIR)/../qemu

include $(LOCAL_PATH)/android/skin/sources.mk

LOCAL_PATH := $(QT_OLD_LOCAL_PATH)

##############################################################################
# qt definitions
ANDROID_QT_CFLAGS := \
    $(foreach inc,$(QT_INCLUDES),-I$(inc)) \
    $(LIBXML2_CFLAGS)

ANDROID_QT_LDFLAGS := $(QT_LDFLAGS)
ANDROID_QT_LDLIBS := $(QT_LDLIBS)

ANDROID_SKIN_CFLAGS += \
    -I$(LIBXML2_INCLUDES) \
    -I$(LIBCURL_INCLUDES) \
    $(ANDROID_QT_CFLAGS) \
    -DCONFIG_QT

ANDROID_SKIN_STATIC_LIBRARIES += \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES_QEMU2) \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \

ANDROID_SKIN_LDLIBS += \
    $(ANDROID_QT_LDLIBS) \
    $(LIBCURL_LDLIBS) \

ANDROID_SKIN_LDFLAGS += \
    $(ANDROID_QT_LDFLAGS)
