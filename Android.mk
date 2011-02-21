LOCAL_PATH:= $(call my-dir)

EMULATOR_TARGET_CPU_DIR := arm
EMULATOR_TARGET_CPU_ABI := armeabi
include $(LOCAL_PATH)/Makefile.android

EMULATOR_TARGET_CPU_DIR := i386
EMULATOR_TARGET_CPU_ABI := x86
include $(LOCAL_PATH)/Makefile.android
