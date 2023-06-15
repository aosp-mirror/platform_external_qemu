// Copyright (C) 2023 The Android Open Source Project
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
#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>

#include "aemu/base/containers/BufferQueue.h"
#include "android/emulation/control/keyboard/EventSender.h"
#include "android/emulation/control/keyboard/dom_key.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {
namespace keyboard {

typedef struct {
    DomCode id;

    // USB keycode:
    //  Upper 16-bits: USB Usage Page.
    //  Lower 16-bits: USB Usage Id: Assigned ID within this usage page.
    uint32_t usb;

    // Contains one of the following:
    //  evdev. used in emulator
    int evdev;

    // Linux xkb
    int xkb;

    //  Windows OEM scancode
    int win;

    //   Mac keycode
    int mac;

    // The UIEvents (aka: DOM4Events) |code| value as defined in:
    // http://www.w3.org/TR/DOM-Level-3-Events-code/
    const char* code;
} KeycodeMapEntry;

#define USB_KEYMAP(usb, evdev, xkb, win, mac, code, id) \
    { DomCode::id, usb, evdev, xkb, win, mac, code }
#define USB_KEYMAP_DECLARATION const KeycodeMapEntry usb_keycode_map[] =
#include "android/emulation/control/keyboard/keycode_converter_data.inc"

#undef USB_KEYMAP
#undef USB_KEYMAP_DECLARATION

struct DomKeyMapEntry {
    DomKey dom_key;
    const char* string;
};

#define DOM_KEY_MAP_DECLARATION const DomKeyMapEntry dom_key_map[] =
#define DOM_KEY_UNI(key, id, value) \
    { DomKey::id, key }
#define DOM_KEY_MAP(key, id, value) \
    { DomKey::id, key }
#include "android/emulation/control/keyboard/dom_key_data.inc"

#undef DOM_KEY_MAP_DECLARATION
#undef DOM_KEY_MAP
#undef DOM_KEY_UNI

using KeyEventQueue = base::BufferQueue<KeyboardEvent>;
// Class that sends Mouse events on the current looper.
class KeyEventSender : public EventSender<KeyboardEvent> {
public:
    KeyEventSender(const AndroidConsoleAgents* const consoleAgents)
        : EventSender<KeyboardEvent>(consoleAgents){};
    ~KeyEventSender() = default;

protected:
    void doSend(const KeyboardEvent request) override;

private:
    void sendKeyCode(int32_t code,
                     const KeyboardEvent::KeyCodeType codeType,
                     const KeyboardEvent::KeyEventType eventType);
    uint32_t domCodeToEvDevKeycode(DomCode key);
    DomCode domKeyAsNonPrintableDomCode(DomKey key);
    DomKey browserKeyToDomKey(std::string key);

    void workerThread();

    // Converts the incoming code type to evdev code that the emulator
    // understands.
    uint32_t convertToEvDev(uint32_t from, KeyboardEvent::KeyCodeType source);

    // Translates a string with one or more Utf8 characters to a sequence of
    // evdev values.
    std::vector<uint32_t> convertUtf8ToEvDev(const std::string& utf8str);
    std::vector<uint32_t> convertUtf8ToEvDev(const char* utf8,
                                             const size_t cnt);
    void sendUtf8String(const std::string& keys);

    int mOffset[KeyboardEvent::KeyCodeType_MAX + 1] = {
            offsetof(KeycodeMapEntry, usb), offsetof(KeycodeMapEntry, evdev),
            offsetof(KeycodeMapEntry, xkb), offsetof(KeycodeMapEntry, win),
            offsetof(KeycodeMapEntry, mac)};
};

}  // namespace keyboard
}  // namespace control
}  // namespace emulation
}  // namespace android
