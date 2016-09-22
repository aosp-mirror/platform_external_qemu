# This build script corresponds to a library containing many definitions
# common to both the guest and the host. They relate to
#
LOCAL_PATH := $(call my-dir)

### emugl_common host library ###########################################

commonSources := \
        crash_reporter.cpp \
        dma_device.cpp \
        feature_control.cpp \
        logging.cpp \
        shared_library.cpp \
        stringparsing.cpp \
        sockets.cpp \
        sync_device.cpp

host_commonSources := $(commonSources)

host_commonLdLibs := $(CXX_STD_LIB)

ifneq (windows,$(BUILD_TARGET_OS))
    host_commonLdLibs += -ldl -lpthread
endif

$(call emugl-begin-static-library,libemugl_common)
LOCAL_SRC_FILES := $(host_commonSources)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/shared)
$(call emugl-export,LDLIBS,$(host_commonLdLibs))
$(call emugl-end-module)

### emugl_common_unittests ##############################################

host_commonSources := \
    shared_library_unittest.cpp \
    stringparsing_unittest.cpp

$(call emugl-begin-executable,emugl$(BUILD_TARGET_SUFFIX)_common_host_unittests)
LOCAL_SRC_FILES := $(host_commonSources)
$(call emugl-import,libemugl_common libemugl_gtest)
$(call local-link-static-c++lib)
$(call emugl-end-module)

$(call emugl-begin-shared-library,lib$(BUILD_TARGET_SUFFIX)emugl_test_shared_library)
LOCAL_SRC_FILES := testing/test_shared_library.cpp
LOCAL_CFLAGS := -fvisibility=default
$(call emugl-end-module)
