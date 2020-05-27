#include <stdint.h>                                      // for uint32_t
#include <cstddef>                                       // for offsetof
#include <string>                                        // for string
#include <vector>                                        // for vector

#include "android/base/containers/BufferQueue.h"         // for BufferQueue
#include "android/console.h"                             // for AndroidConso...
#include "android/emulation/control/keyboard/dom_key.h"  // for DomKey, DomCode
#include "emulator_controller.pb.h"                      // for KeyboardEvent


namespace android {
namespace base {
class Looper;
}  // namespace base

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

class EmulatorKeyEventSender {
public:
    EmulatorKeyEventSender(const AndroidConsoleAgents* const consoleAgents);
    ~EmulatorKeyEventSender();
    void send(const KeyboardEvent* request);
    void sendOnThisThread(const KeyboardEvent* request);

private:
    void doSend(const KeyboardEvent* request);
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
    std::vector<uint32_t> convertUtf8ToEvDev(const char* utf8, const size_t cnt);
    void sendUtf8String(const std::string& keys);

    int mOffset[KeyboardEvent::KeyCodeType_MAX + 1] = {
            offsetof(KeycodeMapEntry, usb), offsetof(KeycodeMapEntry, evdev),
            offsetof(KeycodeMapEntry, xkb), offsetof(KeycodeMapEntry, win),
            offsetof(KeycodeMapEntry, mac)};

    const AndroidConsoleAgents* const mAgents;
    base::Looper* mLooper;
};

}  // namespace keyboard
}  // namespace control
}  // namespace emulation
}  // namespace android
