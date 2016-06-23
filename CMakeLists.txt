# Build options
#
# CMAKE_SYSTEM_NAME
#   (default) ${CMAKE_HOST_NAME}
#   TODO(vharron): Windows - allow Linux->Windows cross compilation
#
# EMU_CRASHUPLOAD
#   (default) NONE
#   NONE - Crash uploading is disabled.
#   STAGING - Upload crashes to staging.
#   PROD - Upload crashes to production.
#
# EMU_BITNESS
#   (default) 64
#   TODO(vharron): 32 -  Create i386 binaries (Only supported for CMAKE_SYSTEM_NAME=Windows)
#   64 - Create x86_64 binaries

cmake_minimum_required(VERSION 3.3)
project (emulator)

# Build system re-write
## Added rpath on Linux/Mac - no more environment variable mess to launch emulators
## Shared libraries in directory with executable
## Windows always looks in executable directory first, then system folder
# CMake build system
## Instead of learning completely custom make functions, learn standard CMake commands
## Delete lots of custom code written in make
## .a .so .dll .exe .m .c .cpp .rc moc/uic/rcc
## library transitive linking
## Potential for hosted Windows builds (emugen?  generator shell scripts?)
# CLion
## Learn one IDE, use it on Windows, Mac, Linux
## Also helps learn IntelliJ/Android Studio
## Refactoring, reliable editing
## Indexed searches
## code navigation ("Find Usages")

# TODO(vharron): qemu1 binaries
# TODO(vharron): launcher (emulator)
# TODO(vharron): add cmake/ninja to emulator manifest
# TODO(vharron): meta build invokes CMake multiple times to build all outputs for release
# TODO(vharron): replace with AUTOMOC/AUTOUIC/AUTORCC
# TODO(vharron): gdbstub generate
# TODO(vharron): prepare symbols for crash reporter
# TODO(vharron): measure build speed
# TODO(vharron): remove qt/plugin path setup
# TODO(vharron): copy Mesa3D libraries

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# define a few global variables to make using paths easier/clearer
set(REPO_PREBUILTS ${CMAKE_CURRENT_LIST_DIR}/../../prebuilts)
set(REPO_PREBUILTS_AEB ${REPO_PREBUILTS}/android-emulator-build)
set(REPO_QEMU ${CMAKE_CURRENT_LIST_DIR})
set(REPO_QEMU2 ${CMAKE_CURRENT_LIST_DIR}-android)

set(CMAKE_VERBOSE_MAKEFILE on) # TEMP DEBUG
#set(CMAKE_SYSTEM_NAME Windows) # TEMP DEBUG

# lets setup our compiler paths for host tools like emugen
if(CMAKE_HOST_SYSTEM MATCHES "Linux")
    set(CMAKE_HOST_C_COMPILER ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-gcc)
    set(CMAKE_HOST_CXX_COMPILER ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-g++)
elseif(CMAKE_HOST_SYSTEM MATCHES "Darwin")
    set(CMAKE_HOST_C_COMPILER ${REPO_PREBUILTS}/clang/darwin-x86/sdk/3.5/bin/clang)
    set(CMAKE_HOST_CXX_COMPILER ${REPO_PREBUILTS}/clang/darwin-x86/sdk/3.5/bin/clang++)
elseif(CMAKE_HOST_SYSTEM MATCHES "Windows")
    # TODO(vharron) support Windows hosted builds
    set(CMAKE_HOST_C_COMPILER TBD)
    set(CMAKE_HOST_CXX_COMPILER TBD)
endif()

set(CMAKE_C_COMPILER ${CMAKE_HOST_C_COMPILER})
set(CMAKE_CXX_COMPILER ${CMAKE_HOST_CXX_COMPILER})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(BUILD_TARGET_TAG linux-x86_64)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=c++11-compat>
    )
    set(QEMU_SYSTEM_LDLIBS
        -Wl,-rpath,$ORIGIN
        -lstdc++
        -lutil
        -lrt
        -lpthread
        )
    set(QEMU_OPT -fPIC)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(BUILD_TARGET_TAG darwin-x86_64)
    set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk")
    set(OSX_DEPLOYMENT_TARGET 10.8)
    set($ENV{SDKROOT} ${CMAKE_OSX_SYSROOT})

    add_compile_options(
        -target x86_64-apple-darwin12.0.0
        -mmacosx-version-min=${OSX_DEPLOYMENT_TARGET}  -DMACOSX_DEPLOYMENT_TARGET=${OSX_DEPLOYMENT_TARGET}
        -D_DARWIN_C_SOURCE=1
        $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=c++14-compat>
        )

    set(QEMU_SYSTEM_LDLIBS
        -Wl,-rpath,$ORIGIN
        -lc++
        -lpthread
        -Wl,-framework,AVFoundation
        -Wl,-framework,Cocoa
        -Wl,-framework,CoreAudio
        -Wl,-framework,CoreMedia
        -Wl,-framework,CoreVideo
        -Wl,-framework,ForceFeedback
        -Wl,-framework,IOKit
        -Wl,-framework,QTKit
        )
    set(QEMU_OPT -fPIC)

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # get rid of -rdynamic
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "") # get rid of -rdynamic
    if(EMU_BITNESS STREQUAL "32")
        set(BUILD_TARGET_TAG windows-x86)
        set(QEMU_OPT -m32)
    else()
        set(BUILD_TARGET_TAG windows-x86_64)
    endif()
    if (CMAKE_HOST_SYSTEM MATCHES "Linux")
        set(CMAKE_C_COMPILER ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-gcc)
        set(CMAKE_CXX_COMPILER ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-g++)
        set(CMAKE_RC_COMPILER ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-windres)

        ################################
        # TODO(vharron): move this out into a proper external toolchain file
        # Begin inline Windows toolchain file
        # here is the target environment located
        set(CMAKE_CROSSCOMPILING TRUE)
        set(CMAKE_FIND_ROOT_PATH ${REPO_PREBUILTS}/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/include)

        # adjust the default behaviour of the FIND_XXX() commands:
        # search headers and libraries in the target environment, search
        # programs in the host environment
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
        # End Windows inline toolchain file
        ################################
        #set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
        #set(CMAKE_EXECUTABLE_SUFFIX ".exe")
    endif()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=c++11-compat>
        )
    set(QEMU_SYSTEM_LDLIBS -lstdc++ -mwindows -mconsole -lwinmm -lws2_32 -liphlpapi)
endif()

add_compile_options(
    ${QEMU_OPT}
    -g # generate debug info in release builds also for analyzing crashes
    -D__STDC_LIMIT_MACROS=1
    -D__STDC_FORMAT_MACROS=1
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE_SOURCE
    -Wall
    -Wno-missing-field-initializers
    -D_GNU_SOURCE=1
    -DCONFIG_CMAKE=1
    )

include_directories(
    ${REPO_PREBUILTS_AEB}/qemu-android-deps/${BUILD_TARGET_TAG}/include
    )

link_libraries(
    -lm
    -L${REPO_PREBUILTS_AEB}/qemu-android-deps/${BUILD_TARGET_TAG}/lib
    ${QEMU_SYSTEM_LDLIBS}
    )

add_subdirectory(distrib/libsparse)
add_subdirectory(distrib/libselinux)
add_subdirectory(distrib/ext4_utils)
add_subdirectory(distrib/googletest)
add_subdirectory(distrib/jpeg-6b)
add_subdirectory(android)
add_subdirectory(distrib/android-emugl)
add_subdirectory(../qemu-android "${CMAKE_CURRENT_BINARY_DIR}/qemu-android")

# Begin QEMU1

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(QEMU1_PLATFORM_OPT
        -DCONFIG_ALSA
        -DCONFIG_ESD
        -DCONFIG_PULSEAUDIO
        -DCONFIG_OSS
        # this is very important, otherwise the generated binaries may
        # not link properly on our build servers
        -fno-stack-protector
        )
    set(QEMU1_PLATFORM_SRC
        # pulse audio
        audio/paaudio.c
        audio/audio_pt_int.c
        audio/wrappers/pulse-audio.c
        # alsa
        audio/alsaaudio.c
        audio/audio_pt_int.c
        audio/wrappers/alsa.c
        # esd
        audio/esdaudio.c
        audio/wrappers/esound.c
        # oss
        audio/ossaudio.c
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(QEMU1_PLATFORM_LIB
        -Wl,-framework,CoreAudio
        )
    set(QEMU1_PLATFORM_OPT
        -DHOST_BSD=1
        -DCONFIG_COREAUDIO
        )
    set(QEMU1_PLATFORM_SRC
        audio/coreaudio.c
        )
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CONFIG_WIN32 1)
    set(QEMU1_PLATFORM_OPT
        -DCONFIG_WINAUDIO
        )
    set(QEMU1_PLATFORM_SRC
        audio/winaudio.c
        block/raw-win32.c
        distrib/mini-glib/src/glib-mini-win32.c
        util/qemu-thread-win32.c
        )
endif()

if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
    set(QEMU1_PLATFORM_LIB
        ${QEMU1_PLATFORM_LIB}
        -ldl
        )
    set(QEMU1_PLATFORM_SRC
        ${QEMU1_PLATFORM_SRC}
        block/raw-posix.c
        posix-aio-compat.c
        util/compatfd.c
        util/qemu-thread-posix.c
        )
endif()

# Generate the QAPI headers and sources from qapi-schema.json
set(QEMU1_GENERATED_QAPI ${CMAKE_CURRENT_BINARY_DIR}/generated/qapi)
file(MAKE_DIRECTORY ${QEMU1_GENERATED_QAPI})
set(QEMU1_QAPI_IN ${CMAKE_CURRENT_LIST_DIR}/qapi-schema.json)
macro(qemu1_qapi_schema_generate SCRIPT FLAG OUT OUT2)
    add_custom_command(
        OUTPUT ${QEMU1_GENERATED_QAPI}/${OUT} ${QEMU1_GENERATED_QAPI}/${OUT2}
        COMMAND python -B ${SCRIPT} --output-dir=${QEMU1_GENERATED_QAPI} ${FLAG} < ${QEMU1_QAPI_IN}
        DEPENDS ${SCRIPT} ${QEMU1_QAPI_IN}
        )
    set(QEMU1_GENERATED_QAPI_C ${QEMU1_GENERATED_QAPI_C} ${QEMU1_GENERATED_QAPI}/${OUT})
    set(QEMU1_GENERATED_QAPI_H ${QEMU1_GENERATED_QAPI_H} ${QEMU1_GENERATED_QAPI}/${OUT2})
endmacro()
set(QEMU1_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/scripts)
#qemu1_qapi_schema_generate(${QEMU1_SCRIPT}/qapi-visit.py -b qapi-visit.c qapi-visit.h)
qemu1_qapi_schema_generate(${QEMU1_SCRIPT}/qapi-types.py -b qapi-types.c qapi-types.h)
#qemu1_qapi_schema_generate(${QEMU1_SCRIPT}/qapi-commands.py -m qmp-marshal.c qmp-commands.h)

# Generate files with hxtool
set(QEMU1_GENERATED_HXTOOL ${CMAKE_CURRENT_BINARY_DIR}/generated/hxtool)
file(MAKE_DIRECTORY ${QEMU1_GENERATED_HXTOOL})
set(HXTOOL ${CMAKE_CURRENT_LIST_DIR}/hxtool)
macro(hxtool IN OUT)
    set(QEMU1_GENERATED_HXTOOL_IN ${CMAKE_CURRENT_LIST_DIR}/${IN})
    set(QEMU1_GENERATED_HXTOOL_OUT ${QEMU1_GENERATED_HXTOOL}/${OUT})
    add_custom_command(
        OUTPUT ${QEMU1_GENERATED_HXTOOL_OUT}
        COMMAND sh ${HXTOOL} -h < ${QEMU1_GENERATED_HXTOOL_IN} > ${QEMU1_GENERATED_HXTOOL_OUT}
        DEPENDS ${HXTOOL} ${QEMU1_GENERATED_HXTOOL_IN}
    )
    set(QEMU1_GENERATED_HXTOOL_FILES ${QEMU1_GENERATED_HXTOOL_FILES} ${QEMU1_GENERATED_HXTOOL_OUT})
endmacro()
hxtool(qemu-options.hx qemu-options.def)
#hxtool(hmp-commands.hx hmp-commands.h)
#hxtool(qmp-commands.hx qmp-commands-old.h)
#hxtool(qemu-img-cmds.hx qemu-img-cmds.h)

add_library(qemu1_common
    ${QEMU1_PLATFORM_SRC}
    ${QEMU1_GENERATED_QAPI_H}
    ${QEMU1_GENERATED_HXTOOL_FILES}
    aio-android.c
    android-qemu1-glue/android_qemud.cpp
    android-qemu1-glue/audio-test.c
    android-qemu1-glue/base/async/Looper.cpp
    android-qemu1-glue/base/files/QemuFileStream.cpp
    android-qemu1-glue/display.c
    android-qemu1-glue/emulation/charpipe.c
    android-qemu1-glue/emulation/CharSerialLine.cpp
    android-qemu1-glue/emulation/serial_line.cpp
    android-qemu1-glue/looper-qemu.cpp
    android-qemu1-glue/qemu-cellular-agent-impl.c
    android-qemu1-glue/qemu-display-agent-impl.cpp
    android-qemu1-glue/qemu-finger-agent-impl.c
    android-qemu1-glue/qemu-location-agent-impl.c
    android-qemu1-glue/qemu-net-agent-impl.c
    android-qemu1-glue/qemu-sensors-agent-impl.c
    android-qemu1-glue/qemu-setup.cpp
    android-qemu1-glue/qemu-telephony-agent-impl.c
    android-qemu1-glue/qemu-window-agent-impl.c
    android-qemu1-glue/telephony/modem_init.cpp
    android-qemu1-glue/utils/stream.cpp
    async.c
    audio/audio.c
    audio/noaudio.c
    audio/wavaudio.c
    audio/wavcapture.c
    audio/mixeng.c
    block.c
    blockdev.c
    block/qcow2.c
    block/qcow2-refcount.c
    block/qcow2-snapshot.c
    block/qcow2-cluster.c
    block/raw.c
    disas/i386.c
    distrib/mini-glib/src/glib-mini.c
    iohandler.c
    ioport.c
    migration-dummy-android.c
    net/net-android.c
    qemu-char.c
    qemu-log.c
    qobject/json-lexer.c
    qobject/json-parser.c
    qobject/json-streamer.c
    qobject/qjson.c
    qobject/qbool.c
    qobject/qdict.c
    qobject/qerror.c
    qobject/qfloat.c
    qobject/qint.c
    qobject/qlist.c
    qobject/qstring.c
    savevm.c
    slirp-android/bootp.c
    slirp-android/cksum.c
    slirp-android/debug.c
    slirp-android/if.c
    slirp-android/ip_icmp.c
    slirp-android/ip_input.c
    slirp-android/ip_output.c
    slirp-android/mbuf.c
    slirp-android/misc.c
    slirp-android/sbuf.c
    slirp-android/slirp.c
    slirp-android/socket.c
    slirp-android/tcp_input.c
    slirp-android/tcp_output.c
    slirp-android/tcp_subr.c
    slirp-android/tcp_timer.c
    slirp-android/tftp.c
    slirp-android/udp.c
    ui/console.c
    ui/d3des.c
    ui/input.c
    ui/vnc-android.c
    util/aes.c
    util/cutils.c
    util/error.c
    util/hexdump.c
    util/iov.c
    util/module.c
    util/notify.c
    util/osdep.c
    util/path.c
    util/qemu-config.c
    util/qemu-error.c
    util/qemu-option.c
    util/qemu-sockets-android.c
    util/unicode.c
    util/yield-android.c
    )
set_target_properties(qemu1_common PROPERTIES LINKER_LANGUAGE C)

target_include_directories(qemu1_common
    PUBLIC
    ${REPO_QEMU}
    slirp-android
    include
    distrib/mini-glib/include
    ${QEMU1_GENERATED_QAPI}
    ${QEMU1_GENERATED_CONFIG}
    PRIVATE
    audio
    )

target_compile_options(qemu1_common
    PUBLIC
    -DHAS_AUDIO
    PRIVATE
    -W
    -Wall
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -fno-strict-aliasing
    -Wno-unused-parameter
    -D_XOPEN_SOURCE=600
    -D_BSD_SOURCE=1
    -DCONFIG_BDRV_WHITELIST=""
    ${QEMU1_PLATFORM_OPT}
    )

target_link_libraries(qemu1_common
    jpeg
    android-emu
    ${QEMU1_PLATFORM_LIB}
    )

#[[

ifeq ($(QEMU_TARGET_XML_SOURCES),)
    QEMU_TARGET_XML_SOURCES := arm-core arm-neon arm-vfp arm-vfp3
    QEMU_TARGET_XML_SOURCES := $(QEMU_TARGET_XML_SOURCES:%=$(LOCAL_PATH)/gdb-xml/%.xml)
endif

$(call start-emulator-library, emulator-libqemu)

# gdbstub-xml.c contains C-compilable arrays corresponding to the content
# of $(LOCAL_PATH)/gdb-xml/, and is generated with the 'feature_to_c.sh' script.
#
intermediates = $(call local-intermediates-dir)
QEMU_GDBSTUB_XML_C = $(intermediates)/gdbstub-xml.c
$(QEMU_GDBSTUB_XML_C): PRIVATE_PATH := $(LOCAL_PATH)
$(QEMU_GDBSTUB_XML_C): PRIVATE_SOURCES := $(TARGET_XML_SOURCES)
$(QEMU_GDBSTUB_XML_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/feature_to_c.sh $@ $(QEMU_TARGET_XML_SOURCES)
$(QEMU_GDBSTUB_XML_C): $(QEMU_TARGET_XML_SOURCES) $(LOCAL_PATH)/feature_to_c.sh
	$(hide) rm -f $@
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(QEMU_GDBSTUB_XML_C)

LOCAL_C_INCLUDES += \
    $(EMULATOR_COMMON_INCLUDES)
    $(BUILD_OBJS_DIR)/build/qemu1-qapi-auto-generated
    $(intermediates) \

LOCAL_CFLAGS := \
    $(EMULATOR_COMMON_CFLAGS)

$(call gen-hw-config-defs)
$(call end-emulator-library)

gen-hx-header = $(eval $(call gen-hx-header-ev,$1,$2,$3))

define gen-hx-header-ev
intermediates := $$(call local-intermediates-dir)

QEMU_HEADER_H := $$(intermediates)/$$2
$$(QEMU_HEADER_H): PRIVATE_PATH := $$(LOCAL_PATH)
$$(QEMU_HEADER_H): PRIVATE_CUSTOM_TOOL = $$(PRIVATE_PATH)/hxtool -h < $$< > $$@
$$(QEMU_HEADER_H): $$(LOCAL_PATH)/$$1 $$(LOCAL_PATH)/hxtool
	$$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $$(QEMU_HEADER_H)
LOCAL_C_INCLUDES += $$(intermediates)
endef
]]

foreach(TARGET_ARCH x86) # arm mips x86_64

    if(TARGET_ARCH MATCHES "(x86|x86_64)")
        set(TCG_TARGET i386)
    else()
        set(TCG_TARGET ${TARGET_ARCH})
    endif()

    if(TARGET_ARCH MATCHES "(x86|x86_64)")
        set(QEMU1_SYSTEM_ARCH_SRC
            hw/i386/pc.c
            hw/i386/smbios.c
            hw/input/pckbd.c
            hw/input/ps2.c
            hw/intc/apic.c
            hw/intc/i8259.c
            hw/intc/ioapic.c
            hw/net/ne2000.c
            hw/nvram/fw_cfg.c
            hw/pci-host/piix.c
            hw/timer/i8254.c
            hw/timer/mc146818rtc.c
            target-i386/cc_helper.c
            target-i386/excp_helper.c
            target-i386/fpu_helper.c
            target-i386/helper.c
            target-i386/int_helper.c
            target-i386/machine.c
            target-i386/mem_helper.c
            target-i386/misc_helper.c
            target-i386/seg_helper.c
            target-i386/smm_helper.c
            target-i386/svm_helper.c
            target-i386/translate.c
            )
        if(CMAKE_SYSTEM_NAME MATCHES "Linux")
            set(QEMU1_SYSTEM_ARCH_SRC
                ${QEMU1_SYSTEM_ARCH_SRC}
                target-i386/kvm.c
                target-i386/kvm-gs-restore.c
                kvm-all.c
                kvm-android.c
                )
        elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
            set(QEMU1_SYSTEM_ARCH_SRC
                ${QEMU1_SYSTEM_ARCH_SRC}
                target-i386/hax-all.c
                target-i386/hax-darwin.c
                )
        elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
            set(QEMU1_SYSTEM_ARCH_SRC
                ${QEMU1_SYSTEM_ARCH_SRC}
                target-i386/hax-all.c
                target-i386/hax-windows.c
                )
        endif()

    elseif(TARGET_ARCH STREQUAL "mips")
        set(QEMU1_SYSTEM_ARCH_SRC
            disas/mips.c
            hw/android/android_mips.c
            hw/android/goldfish/interrupt.c
            hw/android/goldfish/timer.c
            hw/mips/cputimer.c
            hw/mips/mips_int.c
            hw/mips/mips_pic.c
            hw/net/smc91c111.c
            target-mips/helper.c
            target-mips/op_helper.c
            target-mips/dsp_helper.c
            target-mips/translate.c
            target-mips/machine.c
            )
    elseif(TARGET_ARCH STREQUAL "arm")
        set(QEMU1_SYSTEM_ARCH_SRC
            disas/arm.c
            hw/android/android_arm.c
            hw/android/goldfish/interrupt.c
            hw/android/goldfish/timer.c
            hw/arm/armv7m.c
            hw/arm/armv7m_nvic.c
            hw/arm/boot.c
            hw/arm/pic.c
            hw/net/smc91c111.c
            target-arm/arm-semi.c
            target-arm/helper.c
            target-arm/iwmmxt_helper.c
            target-arm/machine.c
            target-arm/op_helper.c
            target-arm/neon_helper.c
            target-arm/translate.c
            )
    endif()

    # What a mess, os-posix.c depends on the exact values of options
    # which are target specific.
    if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
        set(QEMU1_SYSTEM_ARCH_SRC
            ${QEMU1_SYSTEM_ARCH_SRC}
            os-posix.c
            util/oslib-posix.c
            )
    elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(QEMU1_SYSTEM_ARCH_SRC
            ${QEMU1_SYSTEM_ARCH_SRC}
            os-win32.c
            tap-win32.c
            util/oslib-win32.c
            )
    endif()

    add_library(emulator-target-${TARGET_ARCH} STATIC
        ${QEMU1_SYSTEM_ARCH_SRC}
        android-qemu1-glue/main.c
        android-qemu1-glue/qemu-battery-agent-impl.c
        android-qemu1-glue/qemu-user-event-agent-impl.c
        android-qemu1-glue/qemu-vm-operations-impl.c
        android-qemu1-glue/skin_qt.cpp
        arch_init.c
        backends/msmouse.c
        code-profile.c
        cpus.c
        cputlb.c
        cpu-exec.c
        disas.c
        dma-helpers.c
        exec.c
        fpu/softfloat.c
        gdbstub.c
        hw/android/goldfish/audio.c
        hw/android/goldfish/battery.c
        hw/android/goldfish/device.c
        hw/android/goldfish/events_device.c
        hw/android/goldfish/fb.c
        hw/android/goldfish/mmc.c
        hw/android/goldfish/nand.c
        hw/android/goldfish/pipe.c
        hw/android/goldfish/profile.c
        hw/android/goldfish/trace.c
        hw/android/goldfish/tty.c
        hw/android/goldfish/vmem.c
        hw/core/dma.c
        hw/core/irq.c
        hw/core/loader.c
        hw/core/qdev.c
        hw/core/sysbus.c
        hw/pci/pci.c
        hw/watchdog/watchdog.c
        log-rotate-android.c
        main-loop.c
        memory-android.c
        monitor-android.c
        qemu-timer.c
        translate-all.c
        tcg/optimize.c
        tcg/tcg.c
        tcg-runtime.c
        ui/keymaps.c
        util/bitmap.c
        util/bitops.c
        util/host-utils.c
        util/qemu-timer-common.c
        vl-android.c
        )

    target_compile_options(emulator-target-${TARGET_ARCH}
        PRIVATE
        -DNEED_CPU_H
        -DTARGET_ARCH="${TARGET_ARCH}"
        # These are required by the TCG engine.
        -fno-PIC
        -fomit-frame-pointer
        -Wno-sign-compare
        )

    target_include_directories(emulator-target-${TARGET_ARCH}
        PRIVATE
        android/config/target-${TARGET_ARCH}
        target-${TCG_TARGET}
        fpu
        tcg
        tcg/${TCG_TARGET}
        ${QEMU1_GENERATED_HXTOOL}
        )

    target_link_libraries(emulator-target-${TARGET_ARCH}
        qemu1_common
        emulator-ui
        android-emu
        -lpng
        emugl_common
        )

    add_executable(emulator-${TARGET_ARCH}
        images/emulator_icon.rc
        )

    force_link_whole_archive(
        QEMU1_FORCE_LINK_WHOLE_ARCHIVE
        qemu1_common
        emulator-target-${TARGET_ARCH}
        )

    target_link_libraries(emulator-${TARGET_ARCH}
        ${QEMU1_FORCE_LINK_WHOLE_ARCHIVE}
        )

endforeach()

#[[
$(call gen-hw-config-defs)
$(call gen-hx-header,qemu-options.hx,qemu-options.def,os-posix.c os-win32.c)
$(call end-emulator-library)

LOCAL_WHOLE_STATIC_LIBRARIES :=
    emulator-libqemu
    emulator-target-$(EMULATOR_TARGET_CPU)
]]
