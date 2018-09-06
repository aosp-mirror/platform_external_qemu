LOCAL_PATH := $(call my-dir)

emulator-end2end-test = \
    $(eval $(call start-emulator-program, $1_end2endtest)) \
	$(eval LOCAL_C_INCLUDES += $(ANDROID_EMU_INCLUDES) $(EMULATOR_GTEST_INCLUDES)) \
    $(eval LOCAL_STATIC_LIBRARIES += android-emu-base emulator-libgtest) \
	$(eval LOCAL_SRC_FILES := end2end/$1.cpp) \
	$(eval $(call end-emulator-program)) \

ifeq ($(BUILD_TARGET_SUFFIX),64)

$(call emulator-end2end-test,HelloEmulator)

endif
