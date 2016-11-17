# Build files for the Google Benchmark library.
#
# For more information see https://github.com/google/benchmark
#
# This assumes the files are under $AOSP/external/google-benchmark
# (if you don't , use repo sync to update your manifest and workspace).
#
# The compat/ directory contains a few compatibility headers for
# cross-compilation with Mingw.

OLD_LOCAL_PATH := $(LOCAL_PATH)

GOOGLE_BENCHMARK_SOURCES_DIR ?= $(LOCAL_PATH)/../google-benchmark
GOOGLE_BENCHMARK_SOURCES_DIR := $(GOOGLE_BENCHMARK_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(GOOGLE_BENCHMARK_SOURCES_DIR))))
    $(error Cannot find Google Benchmark sources directory: $(GOOGLE_BENCHMARK_SOURCES_DIR))
endif

LOCAL_PATH := $(GOOGLE_BENCHMARK_SOURCES_DIR)

GOOGLE_BENCHMARK_INCLUDES := $(LOCAL_PATH)/include
GOOGLE_BENCHMARK_STATIC_LIBRARIES := emulator-gbench
GOOGLE_BENCHMARK_LDLIBS :=

# On Windows, we need to use our custom regex library.
ifeq (windows,$(BUILD_TARGET_OS))
GOOGLE_BENCHMARK_LDLIBS += -lshlwapi
GOOGLE_BENCHMARK_STATIC_LIBRARIES += $(REGEX_WIN32_STATIC_LIBRARIES)
endif

# Define the emulator-gbench library.
# Note that users should use GOOGLE_BENCHMARK_INCLUDES, GOOGLE_BENCHMARK_STATIC_LIBRARIES
# and GOOGLE_BENCHMARK_LDLIBS to properly use it.
$(call start-emulator-library,emulator-gbench)

LOCAL_C_INCLUDES := \
    $(GOOGLE_BENCHMARK_INCLUDES) \

# NOTE HAVE_STD_REGEX doesn't seem to work with our toolchains.
LOCAL_CFLAGS += -DHAVE_POSIX_REGEX=1

LOCAL_SRC_FILES := \
    src/benchmark.cc \
    src/colorprint.cc \
    src/commandlineflags.cc \
    src/complexity.cc \
    src/console_reporter.cc \
    src/csv_reporter.cc \
    src/json_reporter.cc \
    src/log.cc \
    src/reporter.cc \
    src/re_posix.cc \
    src/sleep.cc \
    src/string_util.cc \
    src/sysinfo.cc \
    src/walltime.cc \

ifeq (windows,$(BUILD_TARGET_OS))

LOCAL_C_INCLUDES += \
    ../qemu/android/third_party/google-benchmark/compat/$(GOOGLE_BENCHMARK_COMPAT_DIR) \
    $(REGEX_WIN32_INCLUDES) \

endif  # BUILD_TARGET_OS == WINDOWS

$(call end-emulator-library)

# Build the google_benchmark[64]_basic_test program, used to check that the
# library links and works properly.
$(call start-emulator-benchmark,google_benchmark$(BUILD_TARGET_SUFFIX)_basic_test)
LOCAL_SRC_FILES := test/basic_test.cc
$(call end-emulator-benchmark)

LOCAL_PATH := $(OLD_LOCAL_PATH)
