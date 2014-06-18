OLD_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(call my-dir)

###########################################################################
###########################################################################
####
####   Breakpad Client Unit Tests
####
###########################################################################
###########################################################################

GOOGLE_BREAKPAD_UNITTESTS_SOURCES := \
    src/common/byte_cursor_unittest.cc \
    src/common/dwarf/bytereader_unittest.cc \
    src/common/dwarf/dwarf2reader_die_unittest.cc \
    src/common/dwarf/dwarf2diehandler_unittest.cc \
    src/common/dwarf/dwarf2reader_cfi_unittest.cc \
    src/common/dwarf_cfi_to_module_unittest.cc \
    src/common/simple_string_dictionary_unittest.cc \
    src/common/dwarf_line_to_module_unittest.cc \
    src/common/memory_unittest.cc \
    src/common/dwarf_cu_to_module_unittest.cc \
    src/common/test_assembler_unittest.cc \
    src/common/module_unittest.cc \
    src/common/stabs_to_module_unittest.cc \
    src/common/memory_range_unittest.cc \
    src/common/stabs_reader_unittest.cc \
    src/client/minidump_file_writer_unittest.cc \

ifeq (linux,$(HOST_OS))
GOOGLE_BREAKPAD_UNITTESTS_SOURCES += \
    src/common/linux/dump_symbols_unittest.cc \
    src/common/linux/linux_libc_support_unittest.cc \
    src/common/linux/elf_core_dump_unittest.cc \
    src/common/linux/safe_readlink_unittest.cc \
    src/common/linux/memory_mapped_file_unittest.cc \
    src/common/linux/synth_elf_unittest.cc \
    src/common/linux/file_id_unittest.cc \
    src/common/linux/elf_symbols_to_module_unittest.cc \
    src/client/linux/minidump_writer/directory_reader_unittest.cc \
    src/client/linux/minidump_writer/minidump_writer_unittest.cc \
    src/client/linux/minidump_writer/linux_core_dumper_unittest.cc \
    src/client/linux/minidump_writer/cpu_set_unittest.cc \
    src/client/linux/minidump_writer/line_reader_unittest.cc \
    src/client/linux/minidump_writer/linux_ptrace_dumper_unittest.cc \
    src/client/linux/minidump_writer/proc_cpuinfo_reader_unittest.cc \
    src/client/linux/handler/exception_handler_unittest.cc
endif  # HOST_OS == linux

ifeq (windows,$(HOST_OS))
GOOGLE_BREAKPAD_UNITTESTS_SOURCES += \
    src/common/windows/omap_unittest.cc
endif  # HOST_OS == windows

ifeq (darwin,$(HOST_OS))
GOOGLE_BREAKPAD_UNITTESTS_SOURCES += \
    src/common/mac/macho_reader_unittest.cc
endif  # HOST_OS == darwin

GOOGLE_BREAKPAD_UNITTESTS_INCLUDES += \
    $(LOCAL_PATH)/support/gtest-include \
    $(LOCAL_PATH)/src/testing/include \
    $(GOOGLE_BREAKPAD_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \

$(call start-emulator-program, breakpad_client_unittests)
LOCAL_SRC_FILES := $(GOOGLE_BREAKPAD_UNITTESTS_SOURCES)
LOCAL_C_INCLUDES += $(GOOGLE_BREAKPAD_UNITTESTS_INCLUDES)
LOCAL_CFLAGS += $(GOOGLE_BREAKPAD_CFLAGS) -O0
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES += $(EMULATOR_GTEST_INCLUDES)
LOCAL_STATIC_LIBRARIES := libbreakpad_client emulator-libgtest
LOCAL_LDLIBS += $(EMULATOR_GTEST_LDLIBS)
$(call end-emulator-program)

####
####  Done.
####

LOCAL_PATH := $(OLD_LOCAL_PATH)
