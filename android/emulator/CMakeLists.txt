prebuilt(QT5)
prebuilt(MESA)
prebuilt(VIRTUALSCENE)
prebuilt(E2FSPROGS)
prebuilt(MACROS)
prebuilt(MSVC)
set(WINDOWS_ICON ../images/emulator_icon.rc)
android_add_executable(
  TARGET emulator INSTALL . LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${WINDOWS_ICON} main-emulator.cpp)
target_link_libraries(emulator PRIVATE android-emu)
target_compile_definitions(
  emulator
  PRIVATE "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}"
          "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
          "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

# Make sure we have the qemu dependencies available
if(LINUX_AARCH64)
  add_dependencies(emulator qemu-system-aarch64 qemu-system-aarch64-headless)
elseif(OPTION_MINBUILD)
  add_dependencies(emulator qemu-system-x86_64 qemu-system-aarch64
                   qemu-system-x86_64-headless qemu-system-aarch64-headless)
else()
  add_dependencies(
    emulator
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
android_license(
  TARGET EMULATOR_BIOS_DEPENDENCIES
  LIBNAME bios
  URL "https://android.googlesource.com/platform/external/qemu-pc-bios/+/refs/heads/master/"
  SPDX "LGPL-2.0-or-later"
  LICENSE
    "https://android.googlesource.com/platform/external/qemu-pc-bios/+/refs/heads/master/README"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.BIOS")

set(EMULATOR_KEYMAP_DEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/pc-bios/keymaps/*>>lib/pc-bios/keymaps")
set(CERT_DEPEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/android/data/ca-bundle.pem>lib/ca-bundle.pem")
set(EMULATOR_HW_DEPENDENCIES
    "${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini>lib/hardware-properties.ini"
)

# Install all the depencies in the right place during build
android_target_dependency(emulator all QT5_SHARED_DEPENDENCIES)
if(NOT OPTION_MINBUILD)
  android_target_dependency(emulator all MESA_DEPENDENCIES)
endif()
android_target_dependency(emulator all EMULATOR_FEATURE_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_KEYMAP_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_BIOS_DEPENDENCIES)
android_target_dependency(emulator all SWIFTSHADER_DEPENDENCIES)
if(NOT LINUX_AARCH64)
  android_target_dependency(emulator all ANGLE_DEPENDENCIES)
endif()
android_target_dependency(emulator all RUNTIME_OS_DEPENDENCIES)
android_target_dependency(emulator all E2FSPROGS_DEPENDENCIES)
android_target_dependency(emulator all CERT_DEPEPENDENCIES)
android_target_dependency(emulator all EMULATOR_HW_DEPENDENCIES)
android_target_dependency(emulator all VIRTUAL_SCENE_DEPENDENCIES)
android_target_dependency(emulator all EMULATOR_MACROS_DEPENDENCIES)
android_target_dependency(emulator all VULKAN_DEPENDENCIES)
android_target_dependency(emulator linux-x86_64 TCMALLOC_OS_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64
                          GLIB2_WINDOWS_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64
                          ANGLE_RENDERER_DEPENDENCIES)
android_target_dependency(emulator windows_msvc-x86_64 MSVC_DEPENDENCIES)
# And install them for a release.
# TODO(b/141205876): prebuilt executables are losing their execute permission on
# installation. Once this issue is fixed, remove all of these
# "android_install_dependency_force_exec()" calls.
android_install_dependency_force_exec(all QT5_SHARED_DEPENDENCIES)
if(NOT OPTION_MINBUILD)
  android_install_dependency(all MESA_DEPENDENCIES)
endif()
android_install_dependency(all EMULATOR_FEATURE_DEPENDENCIES)
android_install_dependency(all EMULATOR_KEYMAP_DEPENDENCIES)
android_install_dependency(all EMULATOR_BIOS_DEPENDENCIES)
android_install_dependency(all SWIFTSHADER_DEPENDENCIES)
if(NOT LINUX_AARCH64)
  android_install_dependency(all ANGLE_DEPENDENCIES)
endif()
android_install_dependency(all RUNTIME_OS_DEPENDENCIES)
android_install_dependency_force_exec(all E2FSPROGS_DEPENDENCIES)
android_install_dependency(all CERT_DEPEPENDENCIES)
android_install_dependency(all EMULATOR_HW_DEPENDENCIES)
android_install_dependency(all VIRTUAL_SCENE_DEPENDENCIES)
android_install_dependency(all EMULATOR_MACROS_DEPENDENCIES)
android_install_dependency(all VULKAN_DEPENDENCIES)
android_install_dependency(linux-x86_64 TCMALLOC_OS_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 GLIB2_WINDOWS_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 ANGLE_RENDERER_DEPENDENCIES)
android_install_dependency(windows_msvc-x86_64 MSVC_DEPENDENCIES)

# Text files have no licenses.
android_license(TARGET "EMULATOR_KEYMAP_DEPENDENCIES" LIBNAME None SPDX None
                LICENSE None LOCAL None)
android_license(TARGET "CERT_DEPEPENDENCIES" LIBNAME None SPDX None LICENSE None
                LOCAL None)
android_license(TARGET "EMULATOR_HW_DEPENDENCIES" LIBNAME None SPDX None
                LICENSE None LOCAL None)
android_license(TARGET "EMULATOR_FEATURE_DEPENDENCIES" LIBNAME None SPDX None
                LICENSE None LOCAL None)

# Bug b/148548657 android_target_dependency(emulator linux-x86_64
# DARWINN_DEPENDENCIES) android_install_dependency(linux-x86_64
# DARWINN_DEPENDENCIES) These are usually nops, but some code generators will
# not automatically binplace these (Xcode!)
android_copy_shared_lib(emulator OpenglRender OpenglRender)

# And set the os specific properties
android_target_properties(emulator all "${RUNTIME_OS_PROPERTIES}")

# Install files we need.
install(FILES ${ANDROID_QEMU2_TOP_DIR}/android-info.txt DESTINATION .)
install(FILES ${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.APACHE2 DESTINATION .
        RENAME LICENSE)
