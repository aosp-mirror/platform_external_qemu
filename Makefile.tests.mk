
# Android skin unit tests

ANDROID_SKIN_UNITTESTS := \
    android/skin/keycode_unittest.cpp \
    android/skin/keycode-buffer_unittest.cpp \
    android/skin/rect_unittest.cpp \
    android/skin/region_unittest.cpp \

$(call start-emulator-program, android$(HOST_SUFFIX)_skin_unittests)
LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \

LOCAL_SRC_FILES := $(ANDROID_SKIN_UNITTESTS)

LOCAL_SRC_FILES += \
    android/gps/GpxParser_unittest.cpp \
    android/gps/internal/GpxParserInternal_unittest.cpp \
    android/gps/internal/KmlParserInternal_unittest.cpp \
    android/gps/KmlParser_unittest.cpp \

LOCAL_C_INCLUDES += \
    $(LIBXML2_INCLUDES) \

LOCAL_CFLAGS += -O0
LOCAL_STATIC_LIBRARIES += \
    emulator-libui \
    emulator-libgtest \
    $(ANDROID_EMU_STATIC_LIBRARIES) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from $(OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)
