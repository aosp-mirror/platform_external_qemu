# EMUGL_EMUGEN := emugen
LOCAL_PATH := $(call my-dir)

$(call start-cmake-project,emugen)
# This is used on the host, not the target
LOCAL_HOST_BUILD=true

 # Define what is produced
PRODUCED_EXECUTABLES:=emugen emugen_unittests

# Define what is consumed
LOCAL_STATIC_LIBRARIES:=
LOCAL_SHARED_LIBRARIES:=

# Define the EMUGL variable..
EMUGL_EMUGEN := $(call local-executable-install-path,emugen)
$(call end-cmake-project)

