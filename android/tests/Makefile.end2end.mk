emulator-end2end-test = \
    $(eval $(call start-emulator-program, $1_end2endtest)) \
    $(eval LOCAL_C_INCLUDES += $(ANDROID_EMU_INCLUDES) $(EMULATOR_GTEST_INCLUDES)) \
    $(eval LOCAL_STATIC_LIBRARIES += $(ANDROID_EMU_STATIC_LIBRARIES) $(EMULATOR_GTEST_STATIC_LIBRARIES)) \
    $(eval LOCAL_SRC_FILES := android/tests/end2end/$1.cpp) \
    $(eval LOCAL_LDLIBS += $(ANDROID_EMU_BASE_LDLIBS)) \
    $(call local-link-static-c++lib) \
    $(eval $(call end-emulator-program)) \

ifeq ($(BUILD_TARGET_SUFFIX),64)

$(call emulator-end2end-test,HelloEmulator)

endif
