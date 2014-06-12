# Copyright 2014 The Android Open Source Project
#
# Android.mk for wearagent
#

LOCAL_PATH:= $(call my-dir)

# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	main.cpp \
	help.cpp

LOCAL_MODULE := wearagent

include $(BUILD_HOST_EXECUTABLE)

