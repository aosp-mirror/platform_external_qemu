// Copyright (C) 2007-2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

// this file implements the support of the new 'hardware control'
// qemud communication channel, which is used by libs/hardware on
// the system image to communicate with the emulator program for
// emulating the following:
//
//   - power management
//   - led(s) brightness
//   - vibrator
//   - flashlight
//
#include "android/hw-control.h"

#include "android/avd/hw-config.h"
#include "android/base/Log.h"
#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

#define D(...) VERBOSE_PRINT(hw_control, __VA_ARGS__)

/* define T_ACTIVE to 1 to debug transport communications */
#define T_ACTIVE 0

#if T_ACTIVE
#define T(...) VERBOSE_PRINT(hw_control, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif

namespace {

struct HwControlState {
    // Button brightness. Ranged from 0-255.
    uint8_t button_brightness = 0;

    // Keyboard brightness. Ranged from 0-255.
    uint8_t keyboard_brightness = 0;

    // Screen LCD brightness. Ranged from 0-255.
    uint8_t lcd_brightness;
};

QemudClient* _hw_control_qemud_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param);

class HwControl;

class HwControlClient {
public:
    explicit HwControlClient(HwControl* hwControl) : mHwControl(hwControl) {}
    static HwControlClient* create(HwControl* hwControl);
    static void close(void* opaque);
    static int load(Stream* stream, QemudClient* qemudClient, void* opaque);
    static void save(Stream* stream, QemudClient* qemudClient, void* opaque);
    static void recv(void* opaque,
                     uint8_t* msg,
                     int msglen,
                     QemudClient* qemudClient);
    void send(const uint8_t* msg, size_t len) const;

    void setQemudClient(QemudClient* client) { mQemudClient = client; }

private:
    HwControl* mHwControl = nullptr;
    QemudClient* mQemudClient = nullptr;
};

class HwControl {
public:
    explicit HwControl(void* client = nullptr,
                       AndroidHwControlFuncs clientFuncs = {});
    void setClient(void* client, AndroidHwControlFuncs clientFuncs);
    void onQuery(std::string_view msg, HwControlClient* client);

    void setBrightness(const std::string& name, uint8_t value);
    void setBrightness(std::string_view args);
    uint32_t getBrightness(std::string_view name) const;

private:
    QemudService* mService = nullptr;
    void* mClient = nullptr;
    AndroidHwControlFuncs mClientFuncs = {};
    HwControlState mHwState = {};
};

HwControl::HwControl(void* client, AndroidHwControlFuncs clientFuncs)
    : mClient(client), mClientFuncs(clientFuncs) {
    mService = qemud_service_register("hw-control", 0, this,
                                      _hw_control_qemud_connect, NULL, NULL);
}

void HwControl::setClient(void* client, AndroidHwControlFuncs clientFuncs) {
    mClient = client;
    mClientFuncs = clientFuncs;
}

void HwControl::onQuery(std::string_view msg, HwControlClient* client) {
    T("%s: query %4lu '%.*s'", __FUNCTION__, msg.size(),
      static_cast<int>(msg.size()), msg.data());

    constexpr std::string_view kSetBrightnessPattern =
            "power:light:brightness:";
    if (msg.find(kSetBrightnessPattern) == 0) {
        std::string_view args = msg.substr(kSetBrightnessPattern.length());
        setBrightness(args);
        return;
    }

    constexpr std::string_view kGetBrightnessPattern =
            "power:light:get-brightness:";
    if (msg.find(kGetBrightnessPattern) == 0) {
        std::string_view args = msg.substr(kGetBrightnessPattern.length());
        uint32_t brightness = getBrightness(args);

        char out[4];
        snprintf(out, sizeof(out), "%03d", brightness);
        client->send(reinterpret_cast<const uint8_t*>(out), strlen(out) + 1);
        return;
    }

    D("%s: query not matched", __func__);
}

void HwControl::setBrightness(const std::string& name, uint8_t value) {
    T("%s: name=%s value=%u", __func__, name.c_str(), value);
    if (name == "lcd_backlight") {
        mHwState.lcd_brightness = value;
    } else if (name == "keyboard_backlight") {
        mHwState.keyboard_brightness = value;
    } else if (name == "button_backlight") {
        mHwState.button_brightness = value;
    } else {
        D("%s: invalid power:light:brightness light name", __func__);
        return;
    }

    if (mClientFuncs.light_brightness != nullptr) {
        mClientFuncs.light_brightness(mClient, name.c_str(), value);
    }
}

void HwControl::setBrightness(std::string_view args) {
    if (auto colon = args.find(':'); colon != std::string_view::npos) {
        std::string name(args.substr(0, colon));
        std::string valueStr(args.substr(colon + 1));
        errno = 0;
        uint64_t value = strtoul(valueStr.c_str(), nullptr, /*base=*/10);
        if (errno != 0) {
            D("%s: invalid power:light:brightness value: \"%s\", errno = %d",
              __func__, valueStr.c_str(), errno);
            return;
        }
        if (value > UINT8_MAX) {
            D("%s: brightness value out of range: %lu", __func__, value);
            return;
        }
        setBrightness(name, static_cast<uint8_t>(value));
    } else {
        D("%s: invalid power:light:brightness command", __func__);
    }
}

uint32_t HwControl::getBrightness(std::string_view name) const {
    T("%s: name=%s", __func__, std::string(name).c_str());
    if (name == "lcd_backlight") {
        return mHwState.lcd_brightness;
    }
    if (name == "keyboard_backlight") {
        return mHwState.keyboard_brightness;
    }
    if (name == "button_backlight") {
        return mHwState.button_brightness;
    }

    D("%s: invalid power:light:get-brightness light name: %s", __func__,
      std::string(name).c_str());
    return 0;
}

// static
HwControlClient* HwControlClient::create(HwControl* hwControl) {
    return new HwControlClient(hwControl);
}

// static
void HwControlClient::close(void* opaque) {
    auto* client = static_cast<HwControlClient*>(opaque);
    delete client;
}

// static
int HwControlClient::load(Stream* stream,
                          QemudClient* qemudClient,
                          void* opaque) {
    auto* client = static_cast<HwControlClient*>(opaque);
    stream_put_be32(stream, client->mHwControl->getBrightness("lcd_backlight"));
    stream_put_be32(stream,
                    client->mHwControl->getBrightness("keyboard_backlight"));
    stream_put_be32(stream,
                    client->mHwControl->getBrightness("button_backlight"));
    return 0;
}

// static
void HwControlClient::save(Stream* stream,
                           QemudClient* qemudClient,
                           void* opaque) {
    auto* client = static_cast<HwControlClient*>(opaque);
    client->mHwControl->setBrightness("lcd_backlight", stream_get_be32(stream));
    client->mHwControl->setBrightness("keyboard_backlight",
                                      stream_get_be32(stream));
    client->mHwControl->setBrightness("button_backlight",
                                      stream_get_be32(stream));
}

// static
void HwControlClient::recv(void* opaque,
                           uint8_t* msg,
                           int msglen,
                           QemudClient* qemudClient) {
    T("%s: hw-control query: %.*s", __FUNCTION__, msglen, msg);
    auto* client = static_cast<HwControlClient*>(opaque);
    client->mHwControl->onQuery(
            std::string_view(reinterpret_cast<const char*>(msg), msglen),
            client);
}

void HwControlClient::send(const uint8_t* msg, size_t len) const {
    T("%s: hw-control query: %.*s", __FUNCTION__, static_cast<int>(len),
      reinterpret_cast<const char*>(msg));
    qemud_client_send(mQemudClient, msg, static_cast<int>(len));
}

std::optional<HwControl> sHwControl;

HwControl& getHwControl() {
    if (!sHwControl) {
        android_hw_control_init();
    }
    return *sHwControl;
}

// Called when a qemud client connects to the service
QemudClient* _hw_control_qemud_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param) {
    QemudClient* qemudClient;
    HwControlClient* client = HwControlClient::create(&getHwControl());
    qemudClient =
            qemud_client_new(service, channel, client_param, client,
                             HwControlClient::recv, HwControlClient::close,
                             HwControlClient::save, HwControlClient::load);
    client->setQemudClient(qemudClient);

    qemud_client_set_framing(qemudClient, 1);
    return qemudClient;
}

}  // namespace

void android_hw_control_init(void) {
    sHwControl = std::make_optional<HwControl>();
    D("%s: hw-control qemud handler initialized", __FUNCTION__);
}

void android_hw_control_set(void* opaque, const AndroidHwControlFuncs* funcs) {
    getHwControl().setClient(opaque, *funcs);
}

void android_hw_control_set_brightness(const char* light_name, uint32_t value) {
    getHwControl().setBrightness(std::string(light_name), value);
}

uint32_t android_hw_control_get_brightness(const char* light_name) {
    return getHwControl().getBrightness(light_name);
}