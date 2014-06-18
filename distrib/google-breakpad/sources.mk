# Build rules for the Google Breakpad client library.

OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

GOOGLE_BREAKPAD_SOURCES := \
    src/client/minidump_file_writer.cc \
    src/common/convert_UTF.c \
    src/common/md5.cc \
    src/common/string_conversion.cc \

GOOGLE_BREAKPAD_CFLAGS :=

GOOGLE_BREAKPAD_INCLUDES := \
    $(LOCAL_PATH)/configs/$(HOST_OS)-x86 \
    $(LOCAL_PATH)/src

GOOGLE_BREAKPAD_INCLUDES_64 := \
    $(LOCAL_PATH)/configs/$(HOST_OS)-x86_64 \
    $(LOCAL_PATH)/src

ifeq (linux,$(HOST_OS))

GOOGLE_BREAKPAD_SOURCES += \
    src/client/linux/crash_generation/crash_generation_client.cc \
    src/client/linux/handler/exception_handler.cc \
    src/client/linux/handler/minidump_descriptor.cc \
    src/client/linux/log/log.cc \
    src/client/linux/minidump_writer/linux_dumper.cc \
    src/client/linux/minidump_writer/linux_ptrace_dumper.cc \
    src/client/linux/minidump_writer/minidump_writer.cc \
    src/common/linux/elfutils.cc \
    src/common/linux/file_id.cc \
    src/common/linux/guid_creator.cc \
    src/common/linux/linux_libc_support.cc \
    src/common/linux/memory_mapped_file.cc \
    src/common/linux/safe_readlink.cc \

endif # HOST_OS == linux

ifeq (windows,$(HOST_OS))

GOOGLE_BREAKPAD_SOURCES += \
    src/client/windows/handler/exception_handler.cc \
    src/client/windows/crash_generation/crash_generation_client.cc \
    src/common/windows/guid_string.cc

GOOGLE_BREAKPAD_CFLAGS += \
    -DBREAKPAD_CUSTOM_STDINT_H='<stdint.h>' \
    -DUNICODE

GOOGLE_BREAKPAD_INCLUDES += \
    $(LOCAL_PATH)/support/windows-include

GOOGLE_BREAKPAD_INCLUDES_64 += \
    $(LOCAL_PATH)/support/windows-include

endif # HOST_OS == windows

ifeq (darwin,$(HOST_OS))

$(warning TODO(digit): Add Darwin Breakpad client library sources)
GOOGLE_BREAKPAD_SOURCES += \

endif # HOST_OS == darwin

$(call start-emulator-library, libbreakpad_client)
LOCAL_SRC_FILES := $(GOOGLE_BREAKPAD_SOURCES)
LOCAL_C_INCLUDES += $(GOOGLE_BREAKPAD_INCLUDES)
LOCAL_CFLAGS += $(GOOGLE_BREAKPAD_CFLAGS)
LOCAL_CPP_EXTENSION := .cc
$(call end-emulator-library)

$(call start-emulator64-library, lib64breakpad_client)
LOCAL_SRC_FILES := $(GOOGLE_BREAKPAD_SOURCES)
LOCAL_C_INCLUDES += $(GOOGLE_BREAKPAD_INCLUDES_64)
LOCAL_CFLAGS += $(GOOGLE_BREAKPAD_CFLAGS)
LOCAL_CPP_EXTENSION := .cc
$(call end-emulator-library)

LOCAL_PATH := $(OLD_LOCAL_PATH)

