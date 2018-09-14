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

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := android/third_party

# TODO(jansene): Use actual cmake from tinyobj?
$(call start-cmake-project,emulator-tinyobjloader)
PRODUCED_STATIC_LIBS := emulator-tinyobjloader
$(call end-cmake-project)

LOCAL_PATH := $(old_LOCAL_PATH)

