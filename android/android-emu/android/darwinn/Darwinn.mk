ifeq ($(BUILD_TARGET_OS),linux)
ifeq ($(BUILD_TARGET_ARCH), x86_64)

# We build darwinn only for 64 bit emulator on linux

    DARWINN_PREBUILT_OLD_LOCAL_PATH := $(LOCAL_PATH)
    LOCAL_PATH := $(call my-dir)
    $(call define-emulator-shared-prebuilt-library,\
        libdarwinn_compiler,\
        $(LOCAL_PATH)/external/libdarwinn_compiler.so)

    $(call define-emulator-shared-prebuilt-library,\
        libdarwinn-driver-none,\
        $(LOCAL_PATH)/external/libreference-driver-none.so)

    $(call define-emulator-prebuilt-library,\
        libdarwinntestdata,\
        $(LOCAL_PATH)/external/libembedded_compiler_test_data.a)

    LIBDARWINN_PREBUILT_SHARED_LIBRARIES := \
        libdarwinn_compiler \
        libdarwinn-driver-none \

    LIBDARWINN_INCLUDES := \
        $(LOCAL_PATH)/external/generated \
        $(LOCAL_PATH)/external/include \
        $(LOCAL_PATH)/external/tests \

    LIBDARWINNTEST_PREBUILT_STATIC_LIBRARIES := \
        libdarwinntestdata \

    DARWINN_CFLAGS := -DDARWINN_PORT_ANDROID_EMULATOR=1 -DDARWINN_CHIP_TYPE=USB -DDARWINN_CHIP_NAME=beagle -DDARWINN_COMPILER_TEST_EXTERNAL

    compute-relative-path = \
        $(shell realpath --relative-to $(DARWINN_PREBUILT_OLD_LOCAL_PATH) $1)

    DARWINN_SERVICE_SRC := \
        $(call compute-relative-path, $(LOCAL_PATH)/darwinn-service.cpp) \

    DARWINN_SERVICE_TEST_SRC := \
        $(call compute-relative-path, $(LOCAL_PATH)/DarwinnPipe_unittest.cpp) \
        $(call compute-relative-path, $(LOCAL_PATH)/MockDarwinnDriver.cpp) \
        $(call compute-relative-path, $(LOCAL_PATH)/MockDarwinnDriverFactory.cpp) \

    DARWINN_COMPILER_TEST_SRC := \
        $(call compute-relative-path, $(LOCAL_PATH)/external/tests/platforms/darwinn/code_generator/api/nnapi/darwinn_compiler_unittest.cpp) \
        $(call compute-relative-path, $(LOCAL_PATH)/external/src/third_party/darwinn/driver/test_util.cc) \

    DARWINN_TEST_THIRDPARTY_SRC := \
        $(call compute-relative-path, $(LOCAL_PATH)/external/src/third_party/darwinn/port/default/logging.cc) \
        $(call compute-relative-path, $(LOCAL_PATH)/external/src/third_party/darwinn/port/default/status.cc) \
        $(call compute-relative-path, $(LOCAL_PATH)/external/src/third_party/darwinn/port/default/statusor.cc) \
        $(call compute-relative-path, $(LOCAL_PATH)/external/src/third_party/darwinn/api/buffer.cc) \

    LOCAL_PATH := $(DARWINN_PREBUILT_OLD_LOCAL_PATH)

endif
endif
