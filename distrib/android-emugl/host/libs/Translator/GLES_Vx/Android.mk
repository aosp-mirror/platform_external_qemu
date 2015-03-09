LOCAL_PATH := $(call my-dir)

##############################################################################
# GLES_V2 host implementation on top of OpenGL

host_common_SRC_FILES := \
     GLESv2Imp.cpp       \
     GLESv2Context.cpp   \
     GLESv2Validate.cpp  \
     ShaderParser.cpp    \
     ProgramData.cpp

### GLES_V2 host implementation, 64-bit
$(call emugl-begin-host-shared-library,libGLES_V2_translator)
$(call emugl-import, libGLcommon)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-end-module)

### GLES_V2 host implementation, 64-bit
$(call emugl-begin-host64-shared-library,lib64GLES_V2_translator)
$(call emugl-import, lib64GLcommon)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-end-module)

##############################################################################
# GLES_CM host implementation on top of GLES_V2_translator

host_common_SRC_FILES :=	\
	GLESv2Context.cpp   	\
	GLEScmContext.cpp   	\
	GLEScmRouting.cpp	\
	GLEScmValidate.cpp  	\
	GLEScmImp.cpp		

### GLES_CM host implementation on top of GL 1.x to 2.0 transformer (es1x)
$(call emugl-begin-host-shared-library,libGLES_CM_translator)
$(call emugl-import,libGLcommon libes1x libGLESv2_dec)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
LOCAL_STATIC_LIBRARIES += libes1x
$(call emugl-end-module)

### GLES_CM host implementation, 64bit on top of GL 1.x to 2.0 transformer (es1x)
$(call emugl-begin-host64-shared-library,lib64GLES_CM_translator)
$(call emugl-import,lib64GLcommon lib64es1x lib64GLESv2_dec)
LOCAL_SRC_FILES := $(host_common_SRC_FILES)
LOCAL_STATIC_LIBRARIES += lib64es1x
$(call emugl-end-module)
