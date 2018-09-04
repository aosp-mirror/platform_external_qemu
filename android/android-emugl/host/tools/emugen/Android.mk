# Determine if the emugen build needs to be builts from
# sources.
LOCAL_PATH:=$(call my-dir)

# Only build emugen on 64-bit targets
ifeq ($(BUILD_TARGET_SUFFIX),64)

$(call emugl-begin-host-executable,emugen)
LOCAL_SRC_FILES := \
    ApiGen.cpp \
    EntryPoint.cpp \
    main.cpp \
    Parser.cpp \
    strUtils.cpp \
    TypeFactory.cpp \

ifeq ($(BUILD_HOST_OS),linux)
  # Make sure libc++.so can be found
  LOCAL_LDFLAGS +=-Wl,-rpath=$(BUILD_OBJS_DIR)/intermediates64
endif

LOCAL_INSTALL := false

$(call emugl-end-module)

endif # BUILD_TARGET_SUFFIX == 64

# The location of the emugen host tool that is used to generate wire
# protocol encoders/ decoders. This variable is used by other emugl modules.
EMUGL_EMUGEN := $(LOCAL_BUILT_MODULE)

$(call emugl-begin-host-executable,emugen_unittests)
LOCAL_SRC_FILES := \
    Parser.cpp \
    Parser_unittest.cpp

LOCAL_INSTALL := false

$(call emugl-import,libemugl_gtest_host)
ifeq ($(BUILD_HOST_OS),linux)
  # Make sure libc++.so can be found
  LOCAL_LDFLAGS +=-Wl,-rpath=$(BUILD_OBJS_DIR)/intermediates64 -m64
endif
$(call emugl-end-module)
