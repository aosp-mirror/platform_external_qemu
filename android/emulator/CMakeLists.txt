prebuilt(QT5)
set(WINDOWS_ICON ../images/emulator_icon.rc)

add_executable(emulator main-emulator.cpp ${WINDOWS_ICON})
target_link_libraries(emulator android-emu)
target_compile_definitions(
  android-emu
  PRIVATE
  -DCONFIG_QT
  -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
  -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER})
set_target_properties(emulator
                      PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(
  EMULATOR_BIOS_DEPENDENCIES
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/bios.bin>lib/pc-bios/bios.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/vgabios-cirrus.bin>lib/pc-bios/vgabios-cirrus.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/vgabios-virtio.bin>lib/pc-bios/vgabios-virtio.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/bios-256k.bin>lib/pc-bios/bios-256k.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/efi-virtio.rom>lib/pc-bios/efi-virtio.rom"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/kvmvapic.bin>lib/pc-bios/kvmvapic.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/linuxboot.bin>lib/pc-bios/linuxboot.bin"
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios/linuxboot_dma.bin>lib/pc-bios/linuxboot_dma.bin"
  )

set(
 SWIFTSHADER_DEPENDENCIES
 "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_TAG}/lib/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}>lib${TARGET_BITS}/gles_swiftshader/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}"
 "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_TAG}/lib/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}>lib${TARGET_BITS}/gles_swiftshader/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}"
 "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_TAG}/lib/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}>lib${TARGET_BITS}/gles_swiftshader/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# Install all the depencies in the right place..
target_prebuilt_dependency(emulator "${QT5_SHARED_DEPENDENCIES}" "")
target_prebuilt_dependency(emulator "${EMULATOR_FEATURE_DEPENDENCIES}" "")
target_prebuilt_dependency(emulator "${EMULATOR_BIOS_DEPENDENCIES}" "")
target_prebuilt_dependency(emulator "${SWIFTSHADER_DEPENDENCIES}" "")
target_prebuilt_dependency(emulator "${RUNTIME_OS_DEPENDENCIES}"
                           "${RUNTIME_OS_PROPERTIES}")
