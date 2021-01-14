#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"

#include <assert.h>  // for assert

#include <cstdint>     // for uint32_t
#include <functional>  // for __base
#include <string>      // for string, oper...
#include <vector>      // for vector

#include "android/base/ArraySize.h"                      // for ARRAY_SIZE
#include "android/base/async/Looper.h"                   // for Looper
#include "android/base/async/ThreadLooper.h"             // for ThreadLooper
#include "android/console.h"                             // for AndroidConso...
#include "android/emulation/control/keyboard/dom_key.h"  // for DomKey, DomCode
#include "android/emulation/control/libui_agent.h"       // for LibuiKeyCode...
#include "android/emulation/control/user_event_agent.h"  // for QAndroidUser...
#include "android/utils/utf8_utils.h"                    // for android_utf8...
#include "emulator_controller.pb.h"                      // for KeyboardEvent

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                               \
    printf("KeyEventSender: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

#define KEYEVENT_QUEUE_LENGTH 128

namespace android {
namespace emulation {
namespace control {
namespace keyboard {

// From
// https://cs.chromium.org/chromium/src/ui/events/keycodes/dom_us_layout_data.h
const struct NonPrintableCodeEntry {
    DomCode dom_code;
    DomKey::Base dom_key;
} kNonPrintableCodeMap[] = {
        {DomCode::ABORT, DomKey::CANCEL},
        {DomCode::AGAIN, DomKey::AGAIN},
        {DomCode::ALT_LEFT, DomKey::ALT},
        {DomCode::ALT_RIGHT, DomKey::ALT},
        {DomCode::ARROW_DOWN, DomKey::ARROW_DOWN},
        {DomCode::ARROW_LEFT, DomKey::ARROW_LEFT},
        {DomCode::ARROW_RIGHT, DomKey::ARROW_RIGHT},
        {DomCode::ARROW_UP, DomKey::ARROW_UP},
        {DomCode::BACKSPACE, DomKey::BACKSPACE},
        {DomCode::BASS_BOOST, DomKey::AUDIO_BASS_BOOST_TOGGLE},
        {DomCode::BRIGHTNESS_DOWN, DomKey::BRIGHTNESS_DOWN},
        {DomCode::BRIGHTNESS_UP, DomKey::BRIGHTNESS_UP},
        // {DomCode::BRIGHTNESS_AUTO, DomKey::_}
        // {DomCode::BRIGHTNESS_MAXIMUM, DomKey::_}
        // {DomCode::BRIGHTNESS_MINIMIUM, DomKey::_}
        // {DomCode::BRIGHTNESS_TOGGLE, DomKey::_}
        {DomCode::BROWSER_BACK, DomKey::BROWSER_BACK},
        {DomCode::BROWSER_FAVORITES, DomKey::BROWSER_FAVORITES},
        {DomCode::BROWSER_FORWARD, DomKey::BROWSER_FORWARD},
        {DomCode::BROWSER_HOME, DomKey::BROWSER_HOME},
        {DomCode::BROWSER_REFRESH, DomKey::BROWSER_REFRESH},
        {DomCode::BROWSER_SEARCH, DomKey::BROWSER_SEARCH},
        {DomCode::BROWSER_STOP, DomKey::BROWSER_STOP},
        {DomCode::CAPS_LOCK, DomKey::CAPS_LOCK},
        {DomCode::CHANNEL_DOWN, DomKey::CHANNEL_DOWN},
        {DomCode::CHANNEL_UP, DomKey::CHANNEL_UP},
        {DomCode::CLOSE, DomKey::CLOSE},
        {DomCode::CLOSED_CAPTION_TOGGLE, DomKey::CLOSED_CAPTION_TOGGLE},
        {DomCode::CONTEXT_MENU, DomKey::CONTEXT_MENU},
        {DomCode::CONTROL_LEFT, DomKey::CONTROL},
        {DomCode::CONTROL_RIGHT, DomKey::CONTROL},
        {DomCode::CONVERT, DomKey::CONVERT},
        {DomCode::COPY, DomKey::COPY},
        {DomCode::CUT, DomKey::CUT},
        {DomCode::DEL, DomKey::DEL},
        {DomCode::EJECT, DomKey::EJECT},
        {DomCode::END, DomKey::END},
        {DomCode::ENTER, DomKey::ENTER},
        {DomCode::ESCAPE, DomKey::ESCAPE},
        {DomCode::EXIT, DomKey::EXIT},
        {DomCode::F1, DomKey::F1},
        {DomCode::F2, DomKey::F2},
        {DomCode::F3, DomKey::F3},
        {DomCode::F4, DomKey::F4},
        {DomCode::F5, DomKey::F5},
        {DomCode::F6, DomKey::F6},
        {DomCode::F7, DomKey::F7},
        {DomCode::F8, DomKey::F8},
        {DomCode::F9, DomKey::F9},
        {DomCode::F10, DomKey::F10},
        {DomCode::F11, DomKey::F11},
        {DomCode::F12, DomKey::F12},
        {DomCode::F13, DomKey::F13},
        {DomCode::F14, DomKey::F14},
        {DomCode::F15, DomKey::F15},
        {DomCode::F16, DomKey::F16},
        {DomCode::F17, DomKey::F17},
        {DomCode::F18, DomKey::F18},
        {DomCode::F19, DomKey::F19},
        {DomCode::F20, DomKey::F20},
        {DomCode::F21, DomKey::F21},
        {DomCode::F22, DomKey::F22},
        {DomCode::F23, DomKey::F23},
        {DomCode::F24, DomKey::F24},
        {DomCode::FIND, DomKey::FIND},
        {DomCode::FN, DomKey::FN},
        {DomCode::FN_LOCK, DomKey::FN_LOCK},
        {DomCode::HELP, DomKey::HELP},
        {DomCode::HOME, DomKey::HOME},
        {DomCode::HYPER, DomKey::HYPER},
        {DomCode::INFO, DomKey::INFO},
        {DomCode::INSERT, DomKey::INSERT},
        // {DomCode::INTL_RO, DomKey::_}
        {DomCode::KANA_MODE, DomKey::KANA_MODE},
        {DomCode::LANG1, DomKey::HANGUL_MODE},
        {DomCode::LANG2, DomKey::HANJA_MODE},
        {DomCode::LANG3, DomKey::KATAKANA},
        {DomCode::LANG4, DomKey::HIRAGANA},
        {DomCode::LANG5, DomKey::ZENKAKU_HANKAKU},
        {DomCode::LAUNCH_APP1, DomKey::LAUNCH_MY_COMPUTER},
        {DomCode::LAUNCH_APP2, DomKey::LAUNCH_CALCULATOR},
        {DomCode::LAUNCH_ASSISTANT, DomKey::LAUNCH_ASSISTANT},
        {DomCode::LAUNCH_AUDIO_BROWSER, DomKey::LAUNCH_MUSIC_PLAYER},
        {DomCode::LAUNCH_CALENDAR, DomKey::LAUNCH_CALENDAR},
        {DomCode::LAUNCH_CONTACTS, DomKey::LAUNCH_CONTACTS},
        {DomCode::LAUNCH_CONTROL_PANEL, DomKey::SETTINGS},
        {DomCode::LAUNCH_INTERNET_BROWSER, DomKey::LAUNCH_WEB_BROWSER},
        {DomCode::LAUNCH_MAIL, DomKey::LAUNCH_MAIL},
        {DomCode::LAUNCH_PHONE, DomKey::LAUNCH_PHONE},
        {DomCode::LAUNCH_SCREEN_SAVER, DomKey::LAUNCH_SCREEN_SAVER},
        {DomCode::LAUNCH_SPREADSHEET, DomKey::LAUNCH_SPREADSHEET},
        // {DomCode::LAUNCH_DOCUMENTS, DomKey::_}
        // {DomCode::LAUNCH_FILE_BROWSER, DomKey::_}
        // {DomCode::LAUNCH_KEYBOARD_LAYOUT, DomKey::_}
        {DomCode::LOCK_SCREEN, DomKey::LAUNCH_SCREEN_SAVER},
        {DomCode::LOG_OFF, DomKey::LOG_OFF},
        {DomCode::MAIL_FORWARD, DomKey::MAIL_FORWARD},
        {DomCode::MAIL_REPLY, DomKey::MAIL_REPLY},
        {DomCode::MAIL_SEND, DomKey::MAIL_SEND},
        {DomCode::MEDIA_FAST_FORWARD, DomKey::MEDIA_FAST_FORWARD},
        {DomCode::MEDIA_LAST, DomKey::MEDIA_LAST},
        // {DomCode::MEDIA_PAUSE, DomKey::MEDIA_PAUSE},
        {DomCode::MEDIA_PLAY, DomKey::MEDIA_PLAY},
        {DomCode::MEDIA_PLAY_PAUSE, DomKey::MEDIA_PLAY_PAUSE},
        {DomCode::MEDIA_RECORD, DomKey::MEDIA_RECORD},
        {DomCode::MEDIA_REWIND, DomKey::MEDIA_REWIND},
        {DomCode::MEDIA_SELECT, DomKey::LAUNCH_MEDIA_PLAYER},
        {DomCode::MEDIA_STOP, DomKey::MEDIA_STOP},
        {DomCode::MEDIA_TRACK_NEXT, DomKey::MEDIA_TRACK_NEXT},
        {DomCode::MEDIA_TRACK_PREVIOUS, DomKey::MEDIA_TRACK_PREVIOUS},
        // {DomCode::MENU, DomKey::_}
        {DomCode::NEW, DomKey::NEW},
        {DomCode::NON_CONVERT, DomKey::NON_CONVERT},
        {DomCode::NUM_LOCK, DomKey::NUM_LOCK},
        {DomCode::NUMPAD_BACKSPACE, DomKey::BACKSPACE},
        {DomCode::NUMPAD_CLEAR, DomKey::CLEAR},
        {DomCode::NUMPAD_ENTER, DomKey::ENTER},
        // {DomCode::NUMPAD_CLEAR_ENTRY, DomKey::_}
        // {DomCode::NUMPAD_MEMORY_ADD, DomKey::_}
        // {DomCode::NUMPAD_MEMORY_CLEAR, DomKey::_}
        // {DomCode::NUMPAD_MEMORY_RECALL, DomKey::_}
        // {DomCode::NUMPAD_MEMORY_STORE, DomKey::_}
        // {DomCode::NUMPAD_MEMORY_SUBTRACT, DomKey::_}
        {DomCode::OPEN, DomKey::OPEN},
        {DomCode::META_LEFT, DomKey::META},
        {DomCode::META_RIGHT, DomKey::META},
        {DomCode::PAGE_DOWN, DomKey::PAGE_DOWN},
        {DomCode::PAGE_UP, DomKey::PAGE_UP},
        {DomCode::PASTE, DomKey::PASTE},
        {DomCode::PAUSE, DomKey::PAUSE},
        {DomCode::POWER, DomKey::POWER},
        {DomCode::PRINT, DomKey::PRINT},
        {DomCode::PRINT_SCREEN, DomKey::PRINT_SCREEN},
        {DomCode::PROGRAM_GUIDE, DomKey::GUIDE},
        {DomCode::PROPS, DomKey::PROPS},
        {DomCode::REDO, DomKey::REDO},
        {DomCode::SAVE, DomKey::SAVE},
        {DomCode::SCROLL_LOCK, DomKey::SCROLL_LOCK},
        {DomCode::SELECT, DomKey::SELECT},
        {DomCode::SELECT_TASK, DomKey::APP_SWITCH},
        {DomCode::SHIFT_LEFT, DomKey::SHIFT},
        {DomCode::SHIFT_RIGHT, DomKey::SHIFT},
        {DomCode::SPEECH_INPUT_TOGGLE, DomKey::SPEECH_INPUT_TOGGLE},
        {DomCode::SPELL_CHECK, DomKey::SPELL_CHECK},
        {DomCode::SUPER, DomKey::SUPER},
        {DomCode::TAB, DomKey::TAB},
        {DomCode::UNDO, DomKey::UNDO},
        {DomCode::VOLUME_DOWN, DomKey::AUDIO_VOLUME_DOWN},
        {DomCode::VOLUME_MUTE, DomKey::AUDIO_VOLUME_MUTE},
        {DomCode::VOLUME_UP, DomKey::AUDIO_VOLUME_UP},
        {DomCode::WAKE_UP, DomKey::WAKE_UP},
        {DomCode::ZOOM_IN, DomKey::ZOOM_IN},
        {DomCode::ZOOM_OUT, DomKey::ZOOM_OUT},
        {DomCode::ZOOM_TOGGLE, DomKey::ZOOM_TOGGLE},

        // Android specific events.
        {DomCode::GO_BACK, DomKey::GO_BACK},        // AC Back in usb
        {DomCode::GO_HOME, DomKey::GO_HOME},        // Same as HOME
        {DomCode::APP_SWITCH, DomKey::APP_SWITCH},  // "Overview"
        // {DomCode::CAMERA, DomKey::CAMERA},          // Camera
};

const size_t kDomKeyMapEntries = ARRAY_SIZE(dom_key_map);
const size_t kKeycodeMapEntries = ARRAY_SIZE(usb_keycode_map);
const size_t kNonPrintableCodeEntries = ARRAY_SIZE(kNonPrintableCodeMap);

EmulatorKeyEventSender::EmulatorKeyEventSender(
        const AndroidConsoleAgents* const consoleAgents)
    : mAgents(consoleAgents) {
    mLooper = android::base::ThreadLooper::get();
}

EmulatorKeyEventSender::~EmulatorKeyEventSender() {}

void EmulatorKeyEventSender::sendKeyCode(
        int32_t code,
        const KeyboardEvent::KeyCodeType codeType,
        const KeyboardEvent::KeyEventType eventType) {
    uint32_t evdev = convertToEvDev(code, codeType);
    if (eventType == KeyboardEvent::keydown ||
        eventType == KeyboardEvent::keypress) {
        DD("Down %d -> evdev 0x%x", code, evdev | 0x400);
        mAgents->user_event->sendKeyCode(evdev | 0x400);
    }
    if (eventType == KeyboardEvent::keyup ||
        eventType == KeyboardEvent::keypress) {
        DD("Up %d -> evdev 0x%x", code, evdev);
        mAgents->user_event->sendKeyCode(evdev);
    }
}  // namespace keyboard

void EmulatorKeyEventSender::send(const KeyboardEvent* request) {
    auto event = KeyboardEvent(*request);
    mLooper->createTask([=]() { doSend(&event); });
}

void EmulatorKeyEventSender::sendOnThisThread(const KeyboardEvent* request) {
    auto event = KeyboardEvent(*request);
    doSend(&event);
}

void EmulatorKeyEventSender::doSend(const KeyboardEvent* request) {
    if (request->key().size() > 0) {
        keyboard::DomKey domkey = browserKeyToDomKey(request->key());
        DD("%s -> domKey: %d, as Non printable: %d",
           request->ShortDebugString().c_str(), (int)domkey,
           (int)domKeyAsNonPrintableDomCode(domkey));

        if (domkey != keyboard::DomKey::NONE) {
            // okay, check if it is a non printable char:
            keyboard::DomCode code = domKeyAsNonPrintableDomCode(domkey);
            std::vector<uint32_t> evdevs;
            if (code == keyboard::DomCode::NONE) {
                auto evdevs = convertUtf8ToEvDev(domkey.ToUtf8());
                auto eventType = request->eventtype();
                if (eventType == KeyboardEvent::keydown ||
                    eventType == KeyboardEvent::keypress) {
                    for (auto evdev : evdevs) {
                        DD("Down %s -> evdev 0x%x",
                           request->ShortDebugString().c_str(), evdev | 0x400);
                        mAgents->user_event->sendKeyCode(evdev | 0x400);
                    }
                }
                if (eventType == KeyboardEvent::keyup ||
                    eventType == KeyboardEvent::keypress) {
                    for (auto evdev : evdevs) {
                        DD("Up %s -> evdev 0x%x",
                           request->ShortDebugString().c_str(), evdev);
                        mAgents->user_event->sendKeyCode(evdev);
                    }
                }
            } else {
                // Nope we have to send the domcode..
                sendKeyCode(domCodeToEvDevKeycode(code), KeyboardEvent::Evdev,
                            request->eventtype());
            }
        }
    }
    if (request->text().size() > 0) {
        sendUtf8String(request->text());
    }

    if (request->keycode() > 0) {
        DD("keycode: %d, codetype:%d, eventtype:%d", request->keycode(),
           request->codetype(), request->eventtype());
        sendKeyCode(request->keycode(), request->codetype(),
                    request->eventtype());
    }
}

void EmulatorKeyEventSender::sendUtf8String(const std::string& utf8) {
    DD("sendUtf8String: %s", utf8.c_str());
    // We need to convert every individual character to a sequence of evdev
    // events. This is due to the fact that a single character can be
    // translated into multiple ev dev events in case a modifier is needed.
    // For example: 'e' -> gives one ev dev event [18]. 'E' ->  gives two
    // [42, 18] (shift + e). The end result is that the sequence "Ee" will be
    // translated as [42, 18, 18]. If we deliver this sequence as down + up, we
    // would see "EE" in android. By splitting this up in individual chars we
    // guarantee that this doesn't happen.
    const char* start = utf8.c_str();  // Guaranteed 0 terminated.
    const char* end = utf8.c_str();
    while (*end != '\0') {
        char ch = *end;
        end++;  // Can point to \0, but not beyond \0

        // For utf-8 we have that if the two high bits set to 10, it's a
        // continuation byte. What we are doing here is checking if the first
        // two bits are not 10 and hence indicate a new character. This means
        // we have found a single utf-8 char between start and end (of at most 4
        // bytes).
        if ((ch & 0xc0) != 0x80) {
            assert(start < end);
            assert(end - start <= 4);
            auto evdevs = convertUtf8ToEvDev(start, end - start);
            // Send down.
            for (auto evdev : evdevs) {
                mAgents->user_event->sendKeyCode(evdev | 0x400);
            }
            // Send up.
            for (auto evdev : evdevs) {
                mAgents->user_event->sendKeyCode(evdev);
            }
            start = end;
        }
    }
}

DomCode EmulatorKeyEventSender::domKeyAsNonPrintableDomCode(DomKey key) {
    for (size_t i = 0; i < kNonPrintableCodeEntries; ++i) {
        if (kNonPrintableCodeMap[i].dom_key == key) {
            return kNonPrintableCodeMap[i].dom_code;
        }
    }
    return DomCode::NONE;
}

uint32_t EmulatorKeyEventSender::domCodeToEvDevKeycode(DomCode key) {
    for (size_t i = 0; i < kKeycodeMapEntries; ++i) {
        if (usb_keycode_map[i].id == key) {
            return usb_keycode_map[i].evdev;
        }
    }
    return 0;
}

std::vector<uint32_t> EmulatorKeyEventSender::convertUtf8ToEvDev(
        const std::string& utf8) {
    return convertUtf8ToEvDev((const char*)utf8.c_str(), utf8.size());
}

std::vector<uint32_t> EmulatorKeyEventSender::convertUtf8ToEvDev(
        const char* utf8,
        const size_t cnt) {
    std::vector<uint32_t> evdevs;
    mAgents->libui->convertUtf8ToKeyCodeEvents(
            (const uint8_t*)utf8, cnt,
            (LibuiKeyCodeSendFunc)[](int* codes, int count, void* context) {
                auto evdevs = reinterpret_cast<std::vector<uint32_t>*>(context);
                for (int i = 0; i < count; i++) {
                    auto code = codes[i];
                    if (code & 0x400) {
                        evdevs->push_back(code & 0x3ff);
                    }
                }
            },
            &evdevs);

    return evdevs;
}

uint32_t EmulatorKeyEventSender::convertToEvDev(
        uint32_t from,
        KeyboardEvent::KeyCodeType source) {
    if (source == KeyboardEvent::Evdev) {
        return from;
    }

    int offset = mOffset[source];
    for (int i = 0; i < kKeycodeMapEntries; i++) {
        int32_t entry = *(int32_t*)((char*)&usb_keycode_map[i] + offset);
        if (entry == from) {
            return usb_keycode_map[i].evdev;
        }
    }
    return 0;
}

DomKey EmulatorKeyEventSender::browserKeyToDomKey(std::string key) {
    if (key.empty()) {
        return DomKey::NONE;
    }

    // Check for standard key names.
    for (size_t i = 0; i < kDomKeyMapEntries; ++i) {
        if (dom_key_map[i].string && key == dom_key_map[i].string) {
            return dom_key_map[i].dom_key;
        }
    }
    if (key == "Dead") {
        // A dead "combining" key; that is, a key which is used in
        // tandem with other keys to generate accented and other
        // modified characters. If pressed by itself, it doesn't
        // generate a character. If you wish to identify which specific
        // dead key was pressed (in cases where more than one exists),
        // you can do so by examining the KeyboardEvent's associated
        // compositionupdate event's  data property.
        return DomKey::DeadKeyFromCombiningCharacter(0xFFFF);
    }
    // Otherwise, if the string contains a single Unicode character,
    // the key value is that character.
    uint32_t character;
    if (key[1] == '\0' && android_utf8_decode((uint8_t*)key.c_str(), key.size(),
                                              &character) >= 0) {
        return DomKey::FromCharacter(character);
    }

    return DomKey::NONE;
}
}  // namespace keyboard
}  // namespace control
}  // namespace emulation
}  // namespace android
