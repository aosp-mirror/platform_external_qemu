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
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <utility>

#include "PacketProtocol.h"
#include "PacketStreamTransport.h"
#include "aemu/base/async/Looper.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/logging/CLog.h"
#include "android/utils/debug.h"
#include "backend/packet_streamer_client.h"
#include "netsim.h"


// clang-format off
// IWYU pragma: begin_keep
extern "C" {
#include "qemu/osdep.h"
#include "chardev/char.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "migration/vmstate.h"
#include "hw/qdev-core.h"
}
// IWYU pragma: end_keep
// clang-format on

#define DEBUG 2
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("chardev-netsim %s:", __func__);         \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x])) {                 \
                printf("%c", buf[x]);                   \
            } else {                                    \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
            }                                           \
        }                                               \
    } while (0);                                        \
    printf("\n");
#endif

// Global netsim configuration.
static struct {
    std::string name;
    std::string default_commands_file;
    std::string controller_properties_file;
} gNetsimConfiguration;

using android::qemu2::PacketStreamChardev;
using android::qemu2::PacketStreamerTransport;

#define TYPE_CHARDEV_NETSIM "chardev-netsim"
#define NETSIM_CHARDEV(obj) \
    OBJECT_CHECK(PacketStreamChardev, (obj), TYPE_CHARDEV_NETSIM)
#define NETSIM_DEVICE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(PacketStreamChardev, obj, TYPE_CHARDEV_NETSIM)

static int netsim_chr_write(Chardev* chr, const uint8_t* buf, int len) {
    DD("Received %d bytes from /dev/vhci", len);
    PacketStreamChardev* netsim = NETSIM_CHARDEV(chr);
    uint64_t consumed = 0;

    if (auto transport = netsim->transport.lock()) {
        DD("Obtained lock, transporting.");
        consumed = transport->fromQemu(buf, len);
    }

    DD("Consumed: %d", consumed);
    return consumed;
}

static void netsim_chr_connect(PacketStreamChardev* netsim) {
    // TODO(jansene): The default_commands_file, and controller_properties_file
    // could be defined as properties on the chardev, v.s. injected here.
    auto channelFactory = []() {
        return netsim::packet::CreateChannel(
                gNetsimConfiguration.default_commands_file,
                gNetsimConfiguration.controller_properties_file);
    };

    auto protocol = android::qemu2::getPacketProtocol(
            netsim->parent.label, gNetsimConfiguration.name);

    auto channel = channelFactory();
    if (!channel) {
        dwarning(
                "Unable to connect to packet streamer, no %s emulation "
                "available",
                netsim->parent.label);
        return;
    }

    auto transport = PacketStreamerTransport::create(
            netsim, std::move(protocol), channel, std::move(channelFactory));
    transport->start();
    netsim->transport = transport;
    dinfo("Activated packet streamer for %s emulation", netsim->parent.label);
}

static void netsim_chr_open(Chardev* chr,
                            ChardevBackend* backend,
                            bool* be_opened,
                            Error** errp) {
    auto netsim = NETSIM_CHARDEV(chr);

    // Directly opening the channel can take >2 seconds.. so let's just
    // assume it succeeds and do it async. If this fails we will simply
    // not respond to any hardware requests.
    android::base::ThreadLooper::get()->scheduleCallback(
            [netsim]() { netsim_chr_connect(netsim); });

    *be_opened = true;
}

static void netsim_chr_cleanup(Object* o) {
    PacketStreamChardev* netsim = NETSIM_CHARDEV(o);
    if (auto transport = netsim->transport.lock()) {
        transport->cancel();
    }

    // Constructed using placement new, so we will manually destruct it.
    netsim->transport.~weak_ptr();
}

static int netsim_chr_machine_done_hook(Chardev* chr) {
    PacketStreamChardev* netsim = NETSIM_CHARDEV(chr);
    if (auto transport = netsim->transport.lock()) {
        transport->qemuConnected();
    }
    return 0;
}

static void netsim_chr_init(Object* obj) {
    PacketStreamChardev* netsim = NETSIM_CHARDEV(obj);
    // Construct the std::weak_ptr member in the pre-allocated memory using
    // placement new
    new (&netsim->transport) std::weak_ptr<PacketStreamerTransport>();
}

static void char_netsim_class_init(ObjectClass* oc, void* data) {
    ChardevClass* cc = CHARDEV_CLASS(oc);
    cc->chr_write = netsim_chr_write;
    cc->open = netsim_chr_open;
    cc->chr_machine_done = netsim_chr_machine_done_hook;
}

static const TypeInfo char_netsim_type_info = {
        .name = TYPE_CHARDEV_NETSIM,
        .parent = TYPE_CHARDEV,
        .instance_size = sizeof(PacketStreamChardev),
        .instance_init = netsim_chr_init,
        .instance_finalize = netsim_chr_cleanup,
        .class_init = char_netsim_class_init,
};

void register_netsim(const std::string address,
                     const std::string rootcanal_default_commands_file,
                     const std::string rootcanal_controller_properties_file,
                     const std::string name) {
    if (!address.empty()) {
        dwarning(
                "The packet stream is unable to connect to %s, (b/265451720) reverting to auto "
                "discovery.",
                address.c_str());
    }
    gNetsimConfiguration.name = name;
    gNetsimConfiguration.default_commands_file =
            rootcanal_default_commands_file;
    gNetsimConfiguration.controller_properties_file =
            rootcanal_controller_properties_file;
}

static void register_types(void) {
    type_register_static(&char_netsim_type_info);
}

type_init(register_types);
