# the following test is made to detect that we were called
# through the 'm' or 'mm' build commands. if not, we use the
# standard QEMU Makefile
#
ifeq ($(DEFAULT_GOAL),droid)
    LOCAL_PATH:= $(call my-dir)

    EMULATOR_TARGET_CPU := arm
    include $(LOCAL_PATH)/Makefile.android

    EMULATOR_TARGET_CPU := i386
    include $(LOCAL_PATH)/Makefile.android

    EMULATOR_TARGET_CPU =
else
    include Makefile.qemu
endif
