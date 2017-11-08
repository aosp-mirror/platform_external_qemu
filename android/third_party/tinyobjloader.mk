###########################################################
###########################################################
###
###  Tinyobjloader library.
###
###  Tinyobjloader is a wavefront obj loader written in C++
###  with no dependency except for STL. This is a
###  header-only library, the implementation is brought in
###  by defining TINYOBJLOADER_IMPLEMENTATION before
###  including the header.

TINYOBJLOADER_SOURCES_DIR := $(LOCAL_PATH)/../tinyobjloader
TINYOBJLOADER_SOURCES_DIR := $(TINYOBJLOADER_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(TINYOBJLOADER_SOURCES_DIR))))
    $(error Cannot find tinyobjloader sources directory: $(TINYOBJLOADER_SOURCES_DIR))
endif

TINYOBJLOADER_INCLUDES := $(TINYOBJLOADER_SOURCES_DIR)

TINYOBJLOADER_SOURCES := \
    tiny_obj_loader.cc

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(TINYOBJLOADER_SOURCES_DIR)

$(call start-emulator-library, emulator-tinyobjloader)
LOCAL_C_INCLUDES += $(TINYOBJLOADER_INCLUDES)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(TINYOBJLOADER_SOURCES)
$(call end-emulator-library)

LOCAL_PATH := $(old_LOCAL_PATH)

