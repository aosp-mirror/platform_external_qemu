/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#define VERBOSE_TAG_LIST                                                       \
    _VERBOSE_TAG(init, "emulator initialization")                              \
    _VERBOSE_TAG(console, "control console")                                   \
    _VERBOSE_TAG(modem, "emulated GSM modem")                                  \
    _VERBOSE_TAG(radio, "emulated GSM AT Command channel")                     \
    _VERBOSE_TAG(keys, "key bindings & presses")                               \
    _VERBOSE_TAG(events, "events sent to the emulator")                        \
    _VERBOSE_TAG(slirp, "internal router/firewall")                            \
    _VERBOSE_TAG(timezone, "host timezone detection")                          \
    _VERBOSE_TAG(socket, "network sockets")                                    \
    _VERBOSE_TAG(proxy, "network proxy support")                               \
    _VERBOSE_TAG(audio, "audio sub-system")                                    \
    _VERBOSE_TAG(audioin, "audio input backend")                               \
    _VERBOSE_TAG(audioout, "audio output backend")                             \
    _VERBOSE_TAG(surface, "video surface support")                             \
    _VERBOSE_TAG(qemud, "qemud multiplexer daemon")                            \
    _VERBOSE_TAG(gps, "emulated GPS")                                          \
    _VERBOSE_TAG(nand_limits, "nand/flash read/write thresholding")            \
    _VERBOSE_TAG(hw_control, "emulated power/flashlight/led/vibrator")         \
    _VERBOSE_TAG(avd_config, "android virtual device configuration")           \
    _VERBOSE_TAG(sensors, "emulated sensors")                                  \
    _VERBOSE_TAG(memcheck, "memory checker")                                   \
    _VERBOSE_TAG(camera, "camera")                                             \
    _VERBOSE_TAG(adevice, "android device connected via port forwarding")      \
    _VERBOSE_TAG(sensors_port, "sensors emulator connected to android device") \
    _VERBOSE_TAG(mtport, "multi-touch emulator connected to android device")   \
    _VERBOSE_TAG(mtscreen, "multi-touch screen emulation")                     \
    _VERBOSE_TAG(gles, "hardware OpenGLES emulation")                          \
    _VERBOSE_TAG(gles1emu, "emulated GLESv1 renderer")                         \
    _VERBOSE_TAG(adbserver, "ADB server")                                      \
    _VERBOSE_TAG(adbclient, "ADB QEMU client")                                 \
    _VERBOSE_TAG(adb, "ADB debugger")                                          \
    _VERBOSE_TAG(asconnector, "Asynchronous socket connector")                 \
    _VERBOSE_TAG(asyncsocket, "Asynchronous socket")                           \
    _VERBOSE_TAG(sdkctlsocket, "Socket tethering to SdkControl server")        \
    _VERBOSE_TAG(updater, "Update checker")                                    \
    _VERBOSE_TAG(metrics, "Metrics reporting")                                 \
    _VERBOSE_TAG(rotation, "Device rotation debugging")                        \
    _VERBOSE_TAG(goldfishsync, "Goldfish Sync Device")                         \
    _VERBOSE_TAG(syncthreads, "HostGPU Sync Threads")                          \
    _VERBOSE_TAG(memory, "Memory Usage Report")                                \
    _VERBOSE_TAG(car, "Emulated car data")                                     \
    _VERBOSE_TAG(record, "Screen recording")                                   \
    _VERBOSE_TAG(snapshot, "Snapshots")                                        \
    _VERBOSE_TAG(virtualscene, "Virtual scene rendering")                      \
    _VERBOSE_TAG(automation, "Automation")                                     \
    _VERBOSE_TAG(offworld, "Offworld")                                         \
    _VERBOSE_TAG(videoinjection, "Video injection")                            \
    _VERBOSE_TAG(foldable, "Foldable Device")                                  \
    _VERBOSE_TAG(curl, "Libcurl requests")                                     \
    _VERBOSE_TAG(car_rotary, "Car rotary controller")                          \
    _VERBOSE_TAG(wifi, "Virtio Wifi")                                          \
    _VERBOSE_TAG(tvremote, "TV remote")                                        \
    _VERBOSE_TAG(time, "Prefix a timestamp when logging")                      \
    _VERBOSE_TAG(ini, "Log details around ini files.")                         \
    _VERBOSE_TAG(bluetooth, "Log bluetooth details.")                          \
    _VERBOSE_TAG(log, "Include timestamp, thread and location in logs")        \
    _VERBOSE_TAG(grpc, "Log grpc calls.")

#include "aemu/base/logging/LogTags.h"

ANDROID_END_HEADER

#ifdef __cplusplus
  #include "aemu/base/logging/Log.h"
#else
  #include "aemu/base/logging/CLog.h"
#endif