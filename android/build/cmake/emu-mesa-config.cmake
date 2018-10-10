# QT Settings ##
get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/mesa/${ANDROID_TARGET_TAG}/lib"
                       ABSOLUTE)

set(MESA_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")

elseif(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(MESA_DEPENDENCIES "${PREBUILT_ROOT}/libGL.so>lib${ANDROID_TARGET_BITS}/gles_mesa/libGL.so")
  set(MESA_DEPENDENCIES "${PREBUILT_ROOT}/libGL.so.1>lib${ANDROID_TARGET_BITS}/gles_mesa/libGL.so.1")
elseif(ANDROID_TARGET_TAG MATCHES "windows.*")
  set(MESA_DEPENDENCIES "${PREBUILT_ROOT}/opengl32.dll>lib${ANDROID_TARGET_BITS}/gles_mesa/mesa_opengl32.dll")
endif()

set(PACKAGE_EXPORT "MESA_DEPENDENCIES;MESA_FOUND")
