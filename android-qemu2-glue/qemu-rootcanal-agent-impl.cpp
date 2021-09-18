// Copyright 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ctype.h>      // for isprint
#include <errno.h>      // for EAGAIN
#include <stdio.h>      // for printf
#include <string.h>     // for memcpy
#include <sys/types.h>  // for ssize_t
#include <algorithm>    // for min
#include <cstdint>      // for uint8_t
#include <mutex>        // for mutex
#include <ostream>      // for char_traits
#include <vector>       // for vector

#include "android/base/Log.h"  // for LogStream...
#include "android/cmdline-option.h"
#include "android/emulation/control/rootcanal_hci_agent.h"  // for QAndroidH...

#ifdef ANDROID_BLUETOOTH
#include "root_canal_qemu.h"
#endif

// clang-format off
extern "C" {
#include "qemu/osdep.h"
#include "chardev/char.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
}
// clang-format on

#ifdef _MSC_VER
#undef send
#undef recv
#endif

/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        dprint("chardev-rootcanal %s:", __func__);      \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                dprint("%c", buf[x]);                   \
            else                                        \
                dprint("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
    } while (0)

#endif

#define TYPE_CHARDEV_ROOTCANAL "chardev-rootcanal"
#define ROOTCANAL_CHARDEV(obj) \
    OBJECT_CHECK(RootcanalChardev, (obj), TYPE_CHARDEV_ROOTCANAL)

static std::vector<uint8_t> sIncomingHciBuffer;
static std::mutex sHciMutex;
static dataAvailableCallback sHciCallback = nullptr;
static void* sOpaque = nullptr;
Chardev* sChrRootcanal;

#ifdef ANDROID_BLUETOOTH
std::unique_ptr<android::bluetooth::Rootcanal> sRootcanal;
#endif

ssize_t rootcanal_recv(uint8_t* buffer, uint64_t bufferSize) {
    errno = 0;
    std::unique_lock<std::mutex> guard(sHciMutex);
    if (sIncomingHciBuffer.empty()) {
        DD("rootcanal_recv: EAGAIN");
        errno = EAGAIN;
        return -1;
    }
    auto readCount = std::min<uint64_t>(sIncomingHciBuffer.size(), bufferSize);
    memcpy(buffer, sIncomingHciBuffer.data(), readCount);

    DD_BUF(buffer, bufferSize);
    sIncomingHciBuffer.erase(sIncomingHciBuffer.begin(),
                             sIncomingHciBuffer.begin() + readCount);
    return readCount;
}

ssize_t rootcanal_send(const uint8_t* buffer, uint64_t bufferSize) {
    qemu_chr_be_write(sChrRootcanal, (uint8_t*)buffer, bufferSize);
    return bufferSize;
};

void rootcanal_register_callback(void* opaque, dataAvailableCallback callback) {
    sOpaque = opaque;
    sHciCallback = callback;
}

size_t rootcanal_available() {
    std::unique_lock<std::mutex> guard(sHciMutex);
    return sIncomingHciBuffer.size();
}

static const QAndroidHciAgent sQAndroidHciAgent = {
        .recv = rootcanal_recv,
        .send = rootcanal_send,
        .available = rootcanal_available,
        .registerDataAvailableCallback = rootcanal_register_callback};

extern "C" const QAndroidHciAgent* const gQAndroidHciAgent = &sQAndroidHciAgent;

static int rootcanal_chr_write(Chardev* chr, const uint8_t* buf, int len) {
    {
        std::unique_lock<std::mutex> guard(sHciMutex);
        sIncomingHciBuffer.insert(sIncomingHciBuffer.end(), buf, buf + len);
        DD_BUF(buf, len);
    }

    if (sHciCallback) {
        (*sHciCallback)(sOpaque);
    }
    return len;
}

static void rootcanal_chr_open(Chardev* chr,
                               ChardevBackend* backend,
                               bool* be_opened,
                               Error** errp) {
    sChrRootcanal = chr;
    *be_opened = false;
#ifdef ANDROID_BLUETOOTH
    android::bluetooth::Rootcanal::Builder builder;
    builder.withHciPort(android_cmdLineOptions->rootcanal_hci_port)
            .withTestPort(android_cmdLineOptions->rootcanal_test_port)
            .withLinkPort(android_cmdLineOptions->rootcanal_link_port)
            .withLinkBlePort(android_cmdLineOptions->rootcanal_link_ble_port)
            .withControllerProperties(
                    android_cmdLineOptions
                            ->rootcanal_controller_properties_file)
            .withCommandFile(
                    android_cmdLineOptions->rootcanal_default_commands_file);
    sRootcanal = builder.build();
    *be_opened = sRootcanal->start();
#endif
    LOG(INFO) << "Rootcanal has " << (*be_opened ? "" : "**NOT**")
              << " been activated.";
}

static void rootcanal_chr_cleanup(Object* o) {
#ifdef ANDROID_BLUETOOTH
    LOG(INFO) << "Closing down rootcanal.";
    sRootcanal->close();
#endif
}

static void char_rootcanal_class_init(ObjectClass* oc, void* data) {
    ChardevClass* cc = CHARDEV_CLASS(oc);
    cc->chr_write = rootcanal_chr_write;
    cc->open = rootcanal_chr_open;
}

static const TypeInfo char_rootcanal_type_info = {
        .name = TYPE_CHARDEV_ROOTCANAL,
        .parent = TYPE_CHARDEV,
        .instance_size = sizeof(Chardev),
        .instance_finalize = rootcanal_chr_cleanup,
        .class_init = char_rootcanal_class_init,
};

static void register_types(void) {
    type_register_static(&char_rootcanal_type_info);
}

type_init(register_types);
