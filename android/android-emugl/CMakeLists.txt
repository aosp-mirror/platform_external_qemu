set(ANDROID_EMUGL_ROOT ${ANDROID_SOURCE_DIR}/android-emugl)

include_directories(
	${ANDROID_SOURCE_DIR}/android-emu
	${ANDROID_EMUGL_ROOT}/shared
	${ANDROID_EMUGL_ROOT}/host/include
	${ANDROID_EMUGL_ROOT}/shared/OpenglCodecCommon
	${ANDROID_EMUGL_ROOT}/host/libs
	${ANDROID_EMUGL_ROOT}/host/include/libOpenglRender
	${ANDROID_EMUGL_ROOT}/host/include/OpenGLESDispatch
	${ANDROID_EMUGL_ROOT}/host/libs/GLESv1_dec
	${ANDROID_EMUGL_ROOT}/host/libs/GLESv2_dec
	${ANDROID_EMUGL_ROOT}/host/libs/libGLSnapshot
	${ANDROID_EMUGL_ROOT}/host/libs/renderControl_dec
	${ANDROID_EMUGL_ROOT}/host/libs/Translator/
	${ANDROID_EMUGL_ROOT}/host/libs/Translator/include
	)

add_subdirectory(host)
add_subdirectory(shared)
