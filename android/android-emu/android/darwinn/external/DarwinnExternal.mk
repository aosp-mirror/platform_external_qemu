ifeq ($(BUILD_TARGET_OS),linux)
    DARWINN_PREBUILT_OLD_LOCAL_PATH := $(LOCAL_PATH)
    LOCAL_PATH := $(call my-dir)
    $(call define-emulator-shared-prebuilt-library,\
        libdarwinn_compiler,\
        $(LOCAL_PATH)/libdarwinn_compiler.so)

    $(call define-emulator-shared-prebuilt-library,\
        libdarwinn-driver-none,\
        $(LOCAL_PATH)/libreference-driver-none.so)

    $(call define-emulator-prebuilt-library,\
        libdarwinntestdata,\
        $(LOCAL_PATH)/libembedded_compiler_test_data.a)

    LIBDARWINN_PREBUILT_SHARED_LIBRARIES := \
        libdarwinn_compiler \
        libdarwinn-driver-none \

    LIBDARWINN_INCLUDES := \
        $(LOCAL_PATH)/generated \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/tests \

    LIBDARWINNTEST_PREBUILT_STATIC_LIBRARIES := \
        libdarwinntestdata \

    DARWINN_CFLAGS := -DDARWINN_PORT_ANDROID_EMULATOR=1 -DDARWINN_CHIP_TYPE=USB -DDARWINN_COMPILER_TEST_EXTERNAL

    compute-relative-path = \
        $(shell realpath --relative-to $(DARWINN_PREBUILT_OLD_LOCAL_PATH) $1)

    DARWINN_COMPILER_TEST_SRC := \
        $(call compute-relative-path, $(LOCAL_PATH)/tests/platforms/darwinn/code_generator/api/nnapi/darwinn_compiler_unittest.cpp) \
        $(call compute-relative-path, $(LOCAL_PATH)/src/third_party/darwinn/driver/test_util.cc) \

    LOCAL_PATH := $(DARWINN_PREBUILT_OLD_LOCAL_PATH)

endif
