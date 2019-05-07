#include <inttypes.h>

#include <cstddef>
#include <string>

#include "android/base/ArraySize.h"
#include "android/console.h"
#include "android/emulation/control/emulator_controller.pb.h"
#include "android/emulation/control/keyboard/dom_key.h"

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

class EmulatorKeyEventSender {
public:
    EmulatorKeyEventSender(const AndroidConsoleAgents* const consoleAgents);
    void send(const KeyboardEvent* request);

private:
    void sendUtf8String(const std::string);
    void sendKeyCode(int32_t code,
                     const KeyboardEvent::KeyCodeType codeType,
                     const KeyboardEvent::KeyEventType eventType);
    uint32_t domCodeToEvDevKeycode(DomCode key);
    DomCode domKeyAsNonPrintableDomCode(DomKey key);
    DomKey browserKeyToDomKey(std::string key);

    // Converts the incoming code type to evdev code that the emulator
    // understands.
    uint32_t convertToEvDev(uint32_t from, KeyboardEvent::KeyCodeType source);
    int mOffset[KeyboardEvent::KeyCodeType_MAX + 1] = {
            offsetof(KeycodeMapEntry, usb), offsetof(KeycodeMapEntry, evdev),
            offsetof(KeycodeMapEntry, xkb), offsetof(KeycodeMapEntry, win),
            offsetof(KeycodeMapEntry, mac)};

    const AndroidConsoleAgents* const mAgents;
};

}  // namespace keyboard
}  // namespace control
}  // namespace emulation
}  // namespace android
