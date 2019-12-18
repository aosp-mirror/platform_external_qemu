prebuilt(QT5)
prebuilt(MESA)
prebuilt(VIRTUALSCENE)
prebuilt(E2FSPROGS)
prebuilt(MACROS)
prebuilt(MSVC)
set(WINDOWS_ICON ../images/emulator_icon.rc)
set(emulator_src main-emulator.cpp ${WINDOWS_ICON})
android_add_executable(emulator)
target_link_libraries(emulator PRIVATE android-emu-base camera android-hw-config)
target_link_libraries(emulator PRIVATE android-emu)


target_compile_definitions(emulator
                           PRIVATE
                           "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
                           "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
                           "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

set(emulator-headless_src
    main-emulator.cpp
    ${WINDOWS_ICON})
android_add_executable(emulator-headless)
target_link_libraries(emulator-headless PRIVATE android-emu camera android-hw-config)

target_compile_definitions(emulator-headless
                           PRIVATE
                           -DCONFIG_HEADLESS
                           "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
                           "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
                           "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

# Make sure we have the qemu dependencies available
if (OPTION_MINBUILD)
  add_dependencies(emulator
                   qemu-system-x86_64
                   qemu-system-aarch64
                   qemu-system-x86_64-headless
                   qemu-system-aarch64-headless)
else()
  add_dependencies(emulator
                   qemu-system-i386
                   qemu-system-x86_64
                   qemu-system-armel
                   qemu-system-aarch64
                   qemu-system-armel-headless
                   qemu-system-x86_64-headless
                   qemu-system-aarch64-headless
                   emulator-crash-service)
endif()

set(PC_BIOS "${ANDROID_QEMU2_TOP_DIR}/pc-bios")
set(EMULATOR_BIOS_DEPENDENCIES
    "${PC_BIOS}/bios.bin>lib/pc-bios/bios.bin"
    "${PC_BIOS}/vgabios-cirrus.bin>lib/pc-bios/vgabios-cirrus.bin"
    "${PC_BIOS}/vgabios-stdvga.bin>lib/pc-bios/vgabios-stdvga.bin"
    "${PC_BIOS}/vgabios-virtio.bin>lib/pc-bios/vgabios-virtio.bin"
    "${PC_BIOS}/bios-256k.bin>lib/pc-bios/bios-256k.bin"
    "${PC_BIOS}/efi-virtio.rom>lib/pc-bios/efi-virtio.rom"
    "${PC_BIOS}/efi-e1000.rom>lib/pc-bios/efi-e1000.rom"
    "${PC_BIOS}/efi-e1000e.rom>lib/pc-bios/efi-e1000e.rom"
    "${PC_BIOS}/kvmvapic.bin>lib/pc-bios/kvmvapic.bin"
    "${PC_BIOS}/linuxboot.bin>lib/pc-bios/linuxboot.bin"
    "${PC_BIOS}/multiboot.bin>lib/pc-bios/multiboot.bin"
    "${PC_BIOS}/linuxboot_dma.bin>lib/pc-bios/linuxboot_dma.bin")

set(EMULATOR_KEYMAP_DEPENDENCIES "${ANDROID_QEMU2_TOP_DIR}/pc-bios/keymaps/*>>lib/pc-bios/keymaps")
set(CERT_DEPEPENDENCIES "${ANDROID_QEMU2_TOP_DIR}/android/data/ca-bundle.pem>lib/ca-bundle.pem")
set(EMULATOR_HW_DEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini>lib/hardware-properties.ini")

# Install all the depencies in the right place during build
android_target_dependency(emulator all QT5_SHARED_DEPENDENCIES)
if(NOT OPTION_MINBUILD)
  android_target_dependency(emulator all MESA_DEPENDENCIES)
endif()
android_target_dependency(emulator all EMULATOR_FEATURE_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_KEYMAP_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_BIOS_DEPENDENCIES)
android_target_dependency(emulator all SWIFTSHADER_DEPENDENCIES)
android_target_dependency(emulator all RUNTIME_OS_DEPENDENCIES)
android_target_dependency(emulator all E2FSPROGS_DEPENDENCIES)
android_target_dependency(emulator all CERT_DEPEPENDENCIES)
android_target_dependency(emulator all EMULATOR_HW_DEPENDENCIES)
android_target_dependency(emulator all VIRTUAL_SCENE_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_MACROS_DEPENDENCIES)
android_target_dependency(emulator all VULKAN_DEPENDENCIES)
android_target_dependency(emulator linux-x86_64 TCMALLOC_OS_DEPENDENCIES)
android_target_dependency(emulator linux-x86_64 DARWINN_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64 GLIB2_WINDOWS_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64 ANGLE_RENDERER_DEPENDENCIES)
android_target_dependency(emulator windows-x86_64 ANGLE_RENDERER_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64 MSVC_DEPENDENCIES)
# And install them for a release.
# TODO(b/141205876): prebuilt executables are losing their execute permission on installation.
# Once this issue is fixed, remove all of these "android_install_dependency_force_exec()" calls.
android_install_dependency_force_exec(all QT5_SHARED_DEPENDENCIES)
if (NOT OPTION_MINBUILD)
  android_install_dependency(all MESA_DEPENDENCIES)
endif()
android_install_dependency(all EMULATOR_FEATURE_DEPENDENCIES)
android_install_dependency(all EMULATOR_KEYMAP_DEPENDENCIES)
android_install_dependency(all EMULATOR_BIOS_DEPENDENCIES)
android_install_dependency(all SWIFTSHADER_DEPENDENCIES)
android_install_dependency(all RUNTIME_OS_DEPENDENCIES)
android_install_dependency_force_exec(all E2FSPROGS_DEPENDENCIES)
android_install_dependency(all CERT_DEPEPENDENCIES)
android_install_dependency(all EMULATOR_HW_DEPENDENCIES)
android_install_dependency(all VIRTUAL_SCENE_DEPENDENCIES)
android_install_dependency(all EMULATOR_MACROS_DEPENDENCIES)
android_install_dependency(all VULKAN_DEPENDENCIES)
android_install_dependency(linux-x86_64 TCMALLOC_OS_DEPENDENCIES)
android_install_dependency(linux-x86_64 DARWINN_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 GLIB2_WINDOWS_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 ANGLE_RENDERER_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 MSVC_DEPENDENCIES)
android_install_dependency(windows-x86_64 ANGLE_RENDERER_DEPENDENCIES)

# These are usually nops, but some code generators will not automatically binplace these (Xcode!)
android_copy_shared_lib(emulator EGL_translator EGL_translator)
android_copy_shared_lib(emulator GLES_V2_translator GLES_V2_translator)
android_copy_shared_lib(emulator GLES_CM_translator GLES_CM_translator)
android_copy_shared_lib(emulator GLES12Translator GLES12Translator)
android_copy_shared_lib(emulator OpenglRender OpenglRender)

# And set the os specific properties
android_target_properties(emulator all "${RUNTIME_OS_PROPERTIES}")

android_install_exe(emulator .)

# Install files we need.
install(FILES ${ANDROID_QEMU2_TOP_DIR}/android-info.txt DESTINATION .)

# Note, these files are identical..
if(WINDOWS)
  install(FILES ${ANDROID_QEMU2_TOP_DIR}/win.source.properties DESTINATION . RENAME source.properties)
elseif(LINUX_X86_64)
  install(FILES ${ANDROID_QEMU2_TOP_DIR}/source.properties DESTINATION .)
elseif(DARWIN_X86_64)
  install(FILES ${ANDROID_QEMU2_TOP_DIR}/win.source.properties DESTINATION . RENAME source.properties)
endif()
