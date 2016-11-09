# Build the regex library for Win32, this is required by google-benchmark.
# The sources come from $AOSP/bionic/libc/upstream-netbsd with commit
# 1665158b7181f9ccd2248eb83e5077f167971801
#
# The compat/ directory contains compatibility headers and sources to make
# everything works correctly. Note that strlcpy() and reallocarr() are
# renamed to __regex_strlcpy() and __regex_reallocarr() to avoid conflicts
# when linking with other libraries that may also define these.

OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

ifeq (windows,$(BUILD_TARGET_OS))

REGEX_WIN32_INCLUDES := $(LOCAL_PATH)/include
REGEX_WIN32_STATIC_LIBRARIES := emulator-regex-win32

$(call start-emulator-library,emulator-regex-win32)

LOCAL_C_INCLUDES += \
    $(REGEX_WIN32_INCLUDES) \
    $(LOCAL_PATH)/compat \

LOCAL_SRC_FILES += \
    compat/reallocarr.c \
    compat/strlcpy.c \
    src/regcomp.c \
    src/regerror.c \
    src/regexec.c \
    src/regfree.c \

$(call end-emulator-library)

endif  # BUILD_TARGET_OS == windows

LOCAL_PATH := $(OLD_LOCAL_PATH)
