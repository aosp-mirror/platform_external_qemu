prebuilt(QT5)
prebuilt(MESA)
set(WINDOWS_ICON ../images/emulator_icon.rc)

add_executable(emulator main-emulator.cpp ${WINDOWS_ICON})
target_link_libraries(emulator android-emu)

# Make sure we have the qemu dependencies available
add_dependencies(emulator qemu-system-i386 qemu-system-x86_64 qemu-system-arm qemu-system-aarch64)

set(PC_BIOS "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/qemu-kernel/x86/pc-bios")
set(EMULATOR_BIOS_DEPENDENCIES
    "${PC_BIOS}/bios.bin>lib/pc-bios/bios.bin"
    "${PC_BIOS}/vgabios-cirrus.bin>lib/pc-bios/vgabios-cirrus.bin"
    "${PC_BIOS}/vgabios-virtio.bin>lib/pc-bios/vgabios-virtio.bin"
    "${PC_BIOS}/bios-256k.bin>lib/pc-bios/bios-256k.bin"
    "${PC_BIOS}/efi-virtio.rom>lib/pc-bios/efi-virtio.rom"
    "${PC_BIOS}/kvmvapic.bin>lib/pc-bios/kvmvapic.bin"
    "${PC_BIOS}/linuxboot.bin>lib/pc-bios/linuxboot.bin"
    "${PC_BIOS}/linuxboot_dma.bin>lib/pc-bios/linuxboot_dma.bin")

set(EMULATOR_KEYMAP_DEPENDENCIES "${ANDROID_QEMU2_TOP_DIR}/pc-bios/keymaps>>lib/pc-bios")
set(CERT_DEPEPENDENCIES "${ANDROID_QEMU2_TOP_DIR}/android/data/ca-bundle.pem>lib/ca-bundle.pem")
set(EMULATOR_HW_DEPEDNECIES "${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini")

# Install all the depencies in the right place..
android_target_dependency(emulator all "${QT5_SHARED_DEPENDENCIES}")
android_target_dependency(emulator all "${MESA_DEPENDENCIES}")
android_target_dependency(emulator all "${EMULATOR_FEATURE_DEPENDENCIES}")
android_target_dependency(emulator all "${EMULATOR_KEYMAP_DEPENDENCIES}")
android_target_dependency(emulator all "${EMULATOR_BIOS_DEPENDENCIES}")
android_target_dependency(emulator all "${SWIFTSHADER_DEPENDENCIES}")
android_target_dependency(emulator all "${RUNTIME_OS_DEPENDENCIES}")
android_target_dependency(emulator all "${E2FSPROGS_DEPENDENCIES}")
android_target_dependency(emulator all "${CERT_DEPEPENDENCIES}")
android_target_dependency(emulator all "${EMULATOR_HW_DEPENDENCIES}")

# Make sure we binplace the qemu binaries.
android_copy_file(emulator $<TARGET_FILE:qemu-system-i386> $<TARGET_FILE_DIR:emulator>/qemu/${ANDROID_TARGET_TAG}/$<TARGET_FILE_NAME:qemu-system-i386>)
android_copy_file(emulator $<TARGET_FILE:qemu-system-x86_64> $<TARGET_FILE_DIR:emulator>/qemu/${ANDROID_TARGET_TAG}/$<TARGET_FILE_NAME:qemu-system-x86_64>)
android_copy_file(emulator $<TARGET_FILE:qemu-system-arm> $<TARGET_FILE_DIR:emulator>/qemu/${ANDROID_TARGET_TAG}/$<TARGET_FILE_NAME:qemu-system-arm>)
android_copy_file(emulator $<TARGET_FILE:qemu-system-aarch64> $<TARGET_FILE_DIR:emulator>/qemu/${ANDROID_TARGET_TAG}/$<TARGET_FILE_NAME:qemu-system-aarch64>)

# These are usually nops, but some code generators will not automatically binplace these (Xcode!)
android_copy_shared_lib(emulator EGL_translator lib64EGL_translator)
android_copy_shared_lib(emulator GLES_V2_translator lib64GLES_V2_translator)
android_copy_shared_lib(emulator GLES_CM_translator lib64GLES_CM_translator)
android_copy_shared_lib(emulator GLES12Translator lib64GLES12Translator)
android_copy_shared_lib(emulator OpenglRender lib64OpenglRender)

# copy test files from test directory
android_copy_test_files(emulator "${android-emu-prebuilt-resources}" resources)


# And set the os specific properties
android_target_properties(emulator all "${RUNTIME_OS_PROPERTIES}")