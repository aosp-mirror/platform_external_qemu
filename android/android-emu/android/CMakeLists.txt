add_subdirectory(avd)
add_subdirectory(base)
add_subdirectory(camera)
# add_subdirectory(crashreport) This is it's own thing
add_subdirectory(emulation)
add_subdirectory(featurecontrol)
add_subdirectory(filesystems)
add_subdirectory(gps)
add_subdirectory(kernel)
add_subdirectory(metrics)
add_subdirectory(misc)
add_subdirectory(network)
add_subdirectory(opengl)
add_subdirectory(protobuf)
add_subdirectory(proxy)
add_subdirectory(qt)
add_subdirectory(skin)
add_subdirectory(telephony)
add_subdirectory(update-check)
add_subdirectory(utils)
add_subdirectory(wear-agent)

include_directories(${CONFIG_HOST_INCLUDE_DIRS})
include_directories(${jpeg-6b_INCLUDE_DIRS})

list(APPEND SOURCES
	adb-server.cpp
	car.cpp
	#cmdline-option_unittest.cpp
	console.c
	console_auth.cpp
	#console_auth_unittest.cpp
	cpu_accelerator.cpp
	curl-support.c
	emuctl-client.cpp
	emulator-window.c
	error-messages.cpp
	#error-messages_unittest.cpp
	ffmpeg-audio-capture.cpp
	ffmpeg-muxer.cpp
	framebuffer.c
	gps.c
	gpu_frame.cpp
	help.c
	hw-control.c
	hw-events.c
	hw-fingerprint.c
	hw-kmsg.c
	hw-lcd.c
	#hw-lcd_unittest.cpp
	hw-qemud.cpp
	hw-sensors.c
	jpeg-compress.c
	loadpng.c
	main-common-ui.c
	main-common.c
	main-emugl.cpp
	main-help.cpp
	main-kernel-parameters.cpp
	main-qemu-parameters.cpp
	multitouch-port.c
	multitouch-screen.c
	opengles.cpp
	openssl-support.cpp
	process_setup.cpp
	qemu-setup.c
	qemu-tcpdump.c
	resource.c
	sdk-controller-socket.c
	sensors-port.c
	shaper.c
	snaphost-android.c
	snapshot.c
	uncompress.cpp
	user-config.cpp
	)

if(APPLE)
elseif(UNIX)
elseif(WIN32)
	list(APPEND SOURCES
	windows_installer.cpp
	#windows_installer_unittest.cpp
	)
endif()

set(GENERATED_SOURCES hw-config-defs.h)
add_custom_command(
  OUTPUT ${GENERATED_SOURCES}
  POST_BUILD
  COMMAND python ${QEMU}/android/scripts/gen-hw-config.py
  ${ANDROID_EMU}/android/avd/hardware-properties.ini ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
  )


add_library(android ${SOURCES} ${GENERATED_SOURCES})
target_link_libraries(android avd base camera emulation featurecontrol filesystems gps kernel metrics network opengl protobuf proxy qt telephony update-check utils wear-agent)
