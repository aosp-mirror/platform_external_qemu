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
#include "android/skin/keyboard.h"

#include <stdbool.h>                                 // for bool, true, false
#include <stdint.h>                                  // for uint8_t, uint32_t
#include <stdio.h>                                   // for fprintf, NULL
#include <string.h>

#include "android/featurecontrol/feature_control.h"  // for feature_is_enabled
#include "android/globals.h"                         // for android_hw
#include "android/skin/android_keycodes.h"           // for KEY_APPSWITCH
#include "android/skin/charmap.h"                    // for skin_charmap_get...
#include "android/skin/keycode-buffer.h"             // for skin_keycode_buf...
#include "android/skin/keycode.h"                    // for skin_key_pair_to...
#include "android/skin/linux_keycodes.h"             // for LINUX_KEY_LEFTSHIFT
#include "android/skin/qt/mac-native-window.h"
#include "android/utils/debug.h"       // for VERBOSE_PRINT
#include "android/utils/system.h"      // for AFREE, ANEW0
#include "android/utils/utf8_utils.h"  // for android_utf8_decode

#define  DEBUG  1

#if DEBUG
#define D(...) VERBOSE_PRINT(keys, __VA_ARGS__)
#else
#  define  D(...)  ((void)0)
#endif

struct SkinKeyboard {
    const SkinCharmap*  charmap;

    SkinRotation        rotation;

    SkinKeycodeBuffer   keycodes[1];
};

#define DEFAULT_ANDROID_CHARMAP  "qwerty2"

static bool skin_key_code_is_arrow(int code) {
    return code == kKeyCodeDpadLeft ||
           code == kKeyCodeDpadRight ||
           code == kKeyCodeDpadUp ||
           code == kKeyCodeDpadDown;
}

static SkinKeyCode
skin_keyboard_key_to_code(SkinKeyboard*  keyboard,
                          int            code,
                          int            mod,
                          int            down)
{
    D("key code=%d mod=%d str=%s",
      code,
      mod,
      skin_key_pair_to_string(code, mod));

    /* first, handle the arrow keys directly */
    if (skin_key_code_is_arrow(code)) {
        code = skin_keycode_rotate(code, -keyboard->rotation);
        D("handling arrow (code=%d mod=%d)", code, mod);
        int  doCapL, doCapR, doAltL, doAltR;

        doCapL = mod & kKeyModLShift;
        doCapR = mod & kKeyModRShift;
        doAltL = mod & kKeyModLAlt;
        doAltR = mod & kKeyModRAlt;

        if (down) {
            if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 1);
            if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 1);
            if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 1);
            if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 1);
        }
        skin_keyboard_add_key_event(keyboard, code, down);

        if (!down) {
            if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 0);
            if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 0);
            if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 0);
            if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 0);
        }
        code = 0;
        return code;
    }

    /* special case for keypad keys, ignore them here if numlock is on */
    if ((mod & kKeyModNumLock) != 0) {
        switch ((int)code) {
            case LINUX_KEY_KP0:
            case LINUX_KEY_KP1:
            case LINUX_KEY_KP2:
            case LINUX_KEY_KP3:
            case LINUX_KEY_KP4:
            case LINUX_KEY_KP5:
            case LINUX_KEY_KP6:
            case LINUX_KEY_KP7:
            case LINUX_KEY_KP8:
            case LINUX_KEY_KP9:
            case LINUX_KEY_KPPLUS:
            case LINUX_KEY_KPMINUS:
            case LINUX_KEY_KPASTERISK:
            case LINUX_KEY_KPSLASH:
            case LINUX_KEY_KPEQUAL:
            case LINUX_KEY_KPDOT:
            case LINUX_KEY_KPENTER:
                return 0;
            default:
                ;
        }
    }

    D("could not handle (code=%d, mod=%d, str=%s)", code, mod,
      skin_key_pair_to_string(code,mod));
    return -1;
}

static SkinKeyMod keycode_to_mod(int key) {
    switch (key) {
    case LINUX_KEY_LEFTALT:
        return kKeyModLAlt;
    case LINUX_KEY_LEFTCTRL:
        return kKeyModLCtrl;
    case LINUX_KEY_LEFTSHIFT:
        return kKeyModLShift;
    }
    return 0;
}

/* If it's running Chrome OS images, enable some special keys for it.
 * Also remap some key code here so the emulator toolbar does the
 * "expected" thing for Chrome OS.
 */
static int map_cros_key(int* code) {
    /* Android emulator uses Qt, and it maps qt key to linux key and
     * send it here.
     * Actually underlying qemu is expecting something else and it calls
     * it as "number". For most keys, "number" is as same as linux key,
     * but it's different for some keys. Since Android emulator doesn't
     * really use those special keys, it doesn't care about this bug. As
     * a quick fix, we remap them here. Also remap some special cros keys
     * here.
     * This mapping can be generated from following 2 steps:
     * linux_to_qcode in ui/input-linux.c maps linux key to qcode
     * qcode_to_number in ui/input-keymap.c maps qcode to "number"
     */
    if ((*code >= LINUX_KEY_F1 && *code <= LINUX_KEY_F10) ||
        *code ==  LINUX_KEY_ESC ||
        *code ==  LINUX_KEY_TAB ||
        *code ==  LINUX_KEY_UP ||
        *code == LINUX_KEY_DOWN ||
        *code == LINUX_KEY_LEFT ||
        *code == LINUX_KEY_RIGHT ||
        *code == LINUX_KEY_VOLUMEUP ||
        *code == LINUX_KEY_VOLUMEDOWN) return 0;

    switch (*code) {
    case LINUX_KEY_BACK:
        *code = LINUX_KEY_F1;
        return 0;
    case LINUX_KEY_HOME:
        *code = LINUX_KEY_LEFTMETA;
        return 0;
    case KEY_APPSWITCH:
        *code = LINUX_KEY_F5;
        return 0;
    case LINUX_KEY_GREEN:
        *code = LINUX_KEY_GRAVE;
        return 0;
    }
    return -1;
}

/* This funcion is used to track the modifier key status and keep it sync
 * between guest and host. It returns the old status for the modifier_key
 */
static SkinKeyMod sync_modifier_key(int modifier_key, SkinKeyboard* kb,
                                    int keycode, int mod, int down) {
    static SkinKeyMod tracked = 0;
    SkinKeyMod mask = keycode_to_mod(modifier_key);
    SkinKeyMod old = tracked & mask;
    if (keycode == modifier_key) {
        if (down) {
            tracked |= mask;
        } else {
            tracked &= ~mask;
        }
        return old;
    }
    /* From our tracking, this modifier key should be in pressed state.
     * But from mod status, it's in released state. This happens sometimes
     * if the release event happens outside of emulator window. Sync status
     * now */
    if (down && old && !(mod & mask)) {
        skin_keyboard_add_key_event(kb, modifier_key, 0);
        tracked &= ~mask;
    }
    return old;
}

static bool has_modifier_key(int keycode, int mod)
{
    if (keycode_to_mod(keycode)) {
        return true;
    }
    /* Don't consider shift here since it will be
     * handeled on kEventTextInput */
    if (mod &(kKeyModLCtrl|kKeyModLAlt)) {
        return true;
    }
    return false;
}

static void process_modifier_key(SkinKeyboard* kb, SkinEvent* ev, int down) {
    if (ev->u.text.mod & kKeyModNumLock) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_NUMLOCK, 1);
        skin_keyboard_add_key_event(kb, LINUX_KEY_NUMLOCK, 0);
    }
    if (ev->u.text.mod & kKeyModCapsLock) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_CAPSLOCK, 1);
        skin_keyboard_add_key_event(kb, LINUX_KEY_CAPSLOCK, 0);
    }
    if (ev->u.text.mod & kKeyModRShift) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_RIGHTSHIFT, down);
    }
    if (ev->u.text.mod & kKeyModLShift) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_LEFTSHIFT, down);
    }
    if (ev->u.text.mod & kKeyModRAlt) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_RIGHTALT, down);
    }
    if (ev->u.text.mod & kKeyModLAlt) {
        skin_keyboard_add_key_event(kb, LINUX_KEY_LEFTALT, down);
    }
}

void
skin_keyboard_process_event(SkinKeyboard*  kb, SkinEvent* ev, int  down)
{
    if (ev->type == kEventTextInput) {
        SkinKeyMod mod = 0;
        if (android_hw->hw_arc) {
            /* skin_keyboard_process_unicode_event will generate capslock
             * key events based on text. Since we've already send shift
             * key events to guest in cros case, we have to make sure
             * it's in released status. Otherwise shift key will
             * interfere capslock. Here, we tell sync_modifier_key
             * there is a key down event without any modifier key.
             * sync_modifier_key will generate one shift release event
             * if the old status for shift is in pressed status. The
             * old status is returned in mod.
             */
            mod = sync_modifier_key(LINUX_KEY_LEFTSHIFT, kb, 0, 0, 1);
        }

        if (!use_keycode_forwarding) {
            // TODO(digit): For each Unicode value in the input text.
            const uint8_t* text = ev->u.text.text;
            const uint8_t* end = text + sizeof(ev->u.text.text);
            while (text < end && *text) {
                uint32_t codepoint = 0;
                int len = android_utf8_decode(text, end - text, &codepoint);
                if (len < 0) {
                    break;
                }
                skin_keyboard_process_unicode_event(kb, codepoint, 1);
                skin_keyboard_process_unicode_event(kb, codepoint, 0);
                text += len;
            }
        } else {
            process_modifier_key(kb, ev, 1);
            D("event type: kTextInput key code=%d mod=0x%x str=%s",
              ev->u.text.keycode, ev->u.text.mod,
              skin_key_pair_to_string(ev->u.text.keycode, ev->u.text.mod));

            if (android_hw->hw_arc) {
                map_cros_key(&ev->u.text.keycode);
            }
            skin_keyboard_add_key_event(kb, ev->u.text.keycode, 1);
            skin_keyboard_add_key_event(kb, ev->u.text.keycode, 0);
            process_modifier_key(kb, ev, 0);
        }

        if (android_hw->hw_arc && mod) {
            /* If the tracked shift status was pressed, restore it by
             * telling sync_modifier_key there is a shift pressed event.
             * Also send out shift pressed event to guest.
             */
            sync_modifier_key(LINUX_KEY_LEFTSHIFT, kb,
                              LINUX_KEY_LEFTSHIFT, 0, 1);
            skin_keyboard_add_key_event(kb, LINUX_KEY_LEFTSHIFT, 1);
        }

        skin_keyboard_flush(kb);
    } else if (ev->type == kEventKeyDown || ev->type == kEventKeyUp) {
        int keycode = ev->u.key.keycode;
        int mod = ev->u.key.mod;

        if (android_hw->hw_arc) {
            sync_modifier_key(LINUX_KEY_LEFTALT, kb, keycode, mod, down);
            sync_modifier_key(LINUX_KEY_LEFTCTRL, kb, keycode, mod, down);
            sync_modifier_key(LINUX_KEY_LEFTSHIFT, kb, keycode, mod, down);
            if (has_modifier_key(keycode, mod) || map_cros_key(&keycode) == 0) {
                skin_keyboard_add_key_event(kb, keycode, down);
                skin_keyboard_flush(kb);
                return;
            }
        }

        /* first, try the keyboard-mode-independent keys */
        int code = skin_keyboard_key_to_code(kb, keycode, mod, down);
        if (code == 0) {
            return;
        }
        if ((int)code > 0) {
            skin_keyboard_add_key_event(kb, code, down);
            skin_keyboard_flush(kb);
            return;
        }

        code = keycode;

        if (code == kKeyCodeAltLeft  || code == kKeyCodeAltRight ||
            code == kKeyCodeCapLeft  || code == kKeyCodeCapRight ||
            code == kKeyCodeSym) {
            skin_keyboard_add_key_event(kb, code, down);
            skin_keyboard_flush(kb);
            return;
        }

        if (code == KEY_APPSWITCH || code == LINUX_KEY_PLAYPAUSE ||
            code == LINUX_KEY_BACK || code == LINUX_KEY_POWER ||
            code == LINUX_KEY_BACKSPACE || code == LINUX_KEY_SOFT1 ||
            code == LINUX_KEY_CENTER || code == LINUX_KEY_REWIND ||
            code == LINUX_KEY_ENTER || code == LINUX_KEY_VOLUMEDOWN ||
            code == LINUX_KEY_FASTFORWARD || code == LINUX_KEY_VOLUMEUP ||
            code == LINUX_KEY_HOME || code == LINUX_KEY_SLEEP ||
            code == KEY_STEM_1 || code == KEY_STEM_2 ||
            code == KEY_STEM_3 || code == KEY_STEM_PRIMARY) {
            skin_keyboard_add_key_event(kb, code, down);
            skin_keyboard_flush(kb);
            return;
        }
        D("ignoring keycode %d", keycode);
    }
}

void
skin_keyboard_set_rotation( SkinKeyboard*     keyboard,
                            SkinRotation      rotation )
{
    keyboard->rotation = (rotation & 3);
}

void
skin_keyboard_add_key_event( SkinKeyboard*  kb,
                             unsigned       code,
                             unsigned       down )
{
    skin_keycode_buffer_add(kb->keycodes, code, down);
}


void
skin_keyboard_flush( SkinKeyboard*  kb )
{
    skin_keycode_buffer_flush(kb->keycodes);
}


int
skin_keyboard_process_unicode_event( SkinKeyboard*  kb,  unsigned int  unicode, int  down )
{
    return skin_charmap_reverse_map_unicode(kb->charmap, unicode, down,
                                            kb->keycodes);
}

static SkinKeyboard* skin_keyboard_create_from_charmap_name(
        const char* charmap_name,
        SkinRotation dpad_rotation,
        SkinKeyCodeFlushFunc keycode_flush) {
    SkinKeyboard*  kb;

    ANEW0(kb);

    kb->charmap = skin_charmap_get_by_name(charmap_name);
    if (!kb->charmap) {
        // Charmap name was not found. Default to "qwerty2" */
        kb->charmap = skin_charmap_get_by_name(DEFAULT_ANDROID_CHARMAP);
        fprintf(stderr,
                "warning, skin requires unknown '%s' charmap, reverting to "
                "'%s'\n",
                charmap_name, kb->charmap->name);
    }
    skin_keycode_buffer_init(kb->keycodes, keycode_flush);
    skin_keyboard_set_rotation(kb, dpad_rotation);
    return kb;
}

SkinKeyboard* skin_keyboard_create(const char* kcm_file_path,
                                   SkinRotation dpad_rotation,
                                   SkinKeyCodeFlushFunc keycode_flush) {
    const char* charmap_name = DEFAULT_ANDROID_CHARMAP;
    char        cmap_buff[SKIN_CHARMAP_NAME_SIZE];

    if (kcm_file_path != NULL) {
        kcm_extract_charmap_name(kcm_file_path, cmap_buff, sizeof cmap_buff);
        charmap_name = cmap_buff;
    }
    return skin_keyboard_create_from_charmap_name(charmap_name, dpad_rotation,
                                                  keycode_flush);
}

void
skin_keyboard_free( SkinKeyboard*  keyboard )
{
    if (keyboard) {
        AFREE(keyboard);
    }
}

#ifndef CONFIG_HEADLESS

#if defined(_WIN32)
#include <windows.h>
const char* skin_keyboard_host_layout_name() {
    char temp[64];
    AMEM_ZERO(temp, sizeof(temp));
    GetKeyboardLayoutNameA(temp);
    return ASTRDUP(temp);
}

// window keyboard layout is represented as ID defined in
// https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-language-pack-default-values
// Thus, we use direct mapping on the basis of best effort. Note that the text
// displayed on screen depends on the android key character mapping.
static const struct {
    const char* native_kb_layout;
    const char* android_kb_layout;
} s_native_to_android_keyboard_map[] = {
        {"00000809", "keyboard_layout_english_uk"},  // United Kingdom
        {"00000409", "keyboard_layout_english_us"},  // United States - English
        {"00020409",
         "keyboard_layout_english_us_intl"},  // United States - International
        {"00010409",
         "keyboard_layout_english_us_dvorak"},  // United States - Dvorak
        {"00000407", "keyboard_layout_german"},
        {"0000040c", "keyboard_layout_french"},
        {"00001009", "keyboard_layout_french_ca"},  // Canadian French
        {"00000419", "keyboard_layout_russian"},
        {"0000040a", "keyboard_layout_spanish"},
        {"0001040a", "keyboard_layout_spanish_latin"},
        {"0000100c", "keyboard_layout_swiss_french"},
        {"00000807", "keyboard_layout_swiss_german"},
        {"0001080c", "keyboard_layout_belgian"},  // Belgian (Comma)
        {"0000080c", "keyboard_layout_belgian"},  // Belgian French
        {"0030402", "keyboard_layout_bulgarian"},
        {"00000410", "keyboard_layout_italian"},
        {"00000406", "keyboard_layout_danish"},
        {"00000414", "keyboard_layout_norwegian"},
        {"0000041d", "keyboard_layout_swedish"},
        {"0000040b", "keyboard_layout_finnish"},
        {"0000041a", "keyboard_layout_croatian_and_slovenian"},  // Croatian
        {"00010405", "keyboard_layout_czech"},  // Czech (QWERTY)
        {"00000425", "keyboard_layout_estonian"},
        {"0000040e", "keyboard_layout_hungarian"},
        {"0000040f", "keyboard_layout_icelandic"},
        {"00000416",
         "keyboard_layout_brazilian"},  // Portuguese (Brazilian ABNT)
        {"00010416",
         "keyboard_layout_brazilian"},  // Portuguese (Brazilian ABNT2)
        {"00000816", "keyboard_layout_portuguese"},
        {"0001041b", "keyboard_layout_slovak"},
        {"00000424", "keyboard_layout_croatian_and_slovenian"},  // Slovenian
        {"0000041f", "keyboard_layout_turkish"},                 // Turkish Q
        {"00000422", "keyboard_layout_ukrainian"},
        {"00010427", "keyboard_layout_lithuanian"},
        {"00020426", "keyboard_layout_latvian"},
        {"00000429", "keyboard_layout_persian"},
        {"0001042c", "keyboard_layout_azerbaijani"},
        {"00000415", "keyboard_layout_polish"},  // Polish (Programmers)
};

#elif defined(__linux__)

#include <X11/XKBlib.h>
#include <xcb/xcb.h>

const char* skin_keyboard_host_layout_name() {
    Display* display = XOpenDisplay(NULL);

    if (!display) {
        fprintf(stderr, "%s: warning: cannot get x11 display info\n", __func__);
        return NULL;
    }

    XkbDescPtr xkb = XkbGetMap(display, XkbAllComponentsMask, XkbUseCoreKbd);

    if (!xkb) {
        fprintf(stderr, "%s: warning: cannot get mod mask info\n", __func__);
        return NULL;
    }

    if (XkbGetNames(display, XkbGroupNamesMask, xkb) != 0) {
        fprintf(stderr, "%s: warning: cannot get xkb group names\n", __func__);
        return NULL;
    }

    XkbStateRec state;
    XkbGetState(display, XkbUseCoreKbd, &state);

    const char* name = XGetAtomName(display, xkb->names->groups[state.group]);

    XkbFreeClientMap(xkb, XkbAllComponentsMask, true);
    XCloseDisplay(display);
    return name;
}

const static struct {
    const char* native_kb_layout;
    const char* android_kb_layout;
} s_native_to_android_keyboard_map[] = {
        {"English (UK)", "keyboard_layout_english_uk"},
        {"English (US)", "keyboard_layout_english_us"},
        {"English (US, alt. intl.)", "keyboard_layout_english_us_intl"},
        {"English (Colemak)", "keyboard_layout_english_us_colemak"},
        {"English (Dvorak)", "keyboard_layout_english_us_dvorak"},
        {"English (Workman)", "keyboard_layout_english_workman"},
        {"German", "keyboard_layout_german"},
        {"French (AZERTY)", "keyboard_layout_french"},
        {"French (Canada)", "keyboard_layout_french_ca"},
        {"Russian", "keyboard_layout_russian"},
        {"Russian (Macintosh)", "keyboard_layout_russian_mac"},
        {"Spanish", "keyboard_layout_spanish"},
        {"Spanish (Latin America)", "keyboard_layout_spanish_latin"},
        {"French (Switzerland)", "keyboard_layout_swiss_french"},
        {"German (Switzerland)", "keyboard_layout_swiss_german"},
        {"Belgian", "keyboard_layout_belgian"},
        {"Bulgarian", "keyboard_layout_bulgarian"},
        {"Italian", "keyboard_layout_italian"},
        {"Danish", "keyboard_layout_danish"},
        {"Norwegian", "keyboard_layout_norwegian"},
        {"Swedish", "keyboard_layout_swedish"},
        {"Finnish", "keyboard_layout_finnish"},
        {"Croatian", "keyboard_layout_croatian_and_slovenian"},
        {"Czech (QWERTY)", "keyboard_layout_czech"},
        {"Estonian", "keyboard_layout_estonian"},
        {"Hungarian", "keyboard_layout_hungarian"},
        {"Icelandic", "keyboard_layout_icelandic"},
        {"Portuguese (Brazil)", "keyboard_layout_brazilian"},
        {"Portuguese", "keyboard_layout_portuguese"},
        {"Slovak (QWERTY)", "keyboard_layout_slovak"},
        {"Slovenian", "keyboard_layout_croatian_and_slovenian"},
        {"Turkish", "keyboard_layout_turkish"},
        //{"Ukrainian", "keyboard_layout_ukrainian"},
        {"Latvian", "keyboard_layout_latvian_qwerty"},
        {"Persian", "keyboard_layout_persian"},
        {"Azerbaijani", "keyboard_layout_azerbaijani"},
        {"Polish", "keyboard_layout_polish"},
};

#else  // __APPLE__
// Defined in mac-native-window.mm
const char* skin_keyboard_host_layout_name() {
    return keyboard_host_layout_name_macImpl();
}

const static struct {
    const char* native_kb_layout;
    const char* android_kb_layout;
} s_native_to_android_keyboard_map[] = {
        {"com.apple.keylayout.British", "keyboard_layout_english_uk"},
        {"com.apple.keylayout.US", "keyboard_layout_english_us"},
        {"com.apple.keylayout.USInternational-PC",
         "keyboard_layout_english_us_intl"},
        {"com.apple.keylayout.Colemak", "keyboard_layout_english_us_colemak"},
        {"com.apple.keylayout.Dvorak", "keyboard_layout_english_us_dvorak"},
        {"com.apple.keylayout.German", "keyboard_layout_german"},
        {"com.apple.keylayout.French", "keyboard_layout_french"},
        {"com.apple.keylayout.Canadian-CSA", "keyboard_layout_french_ca"},
        {"com.apple.keylayout.Russian", "keyboard_layout_russian_mac"},
        {"com.apple.keylayout.RussianWin", "keyboard_layout_russian"},
        {"com.apple.keylayout.Spanish", "keyboard_layout_spanish"},
        {"com.apple.keylayout.Spanish-ISO", "keyboard_layout_spanish_latin"},
        {"com.apple.keylayout.SwissFrench", "keyboard_layout_swiss_french"},
        {"com.apple.keylayout.SwissGerman", "keyboard_layout_swiss_german"},
        {"com.apple.keylayout.Belgian", "keyboard_layout_belgian"},
        {"com.apple.keylayout.Bulgarian", "keyboard_layout_bulgarian"},
        {"com.apple.keylayout.Italian", "keyboard_layout_italian"},
        {"com.apple.keylayout.Danish", "keyboard_layout_danish"},
        {"com.apple.keylayout.Norwegian", "keyboard_layout_norwegian"},
        {"com.apple.keylayout.Swedish", "keyboard_layout_swedish"},
        {"com.apple.keylayout.Finnish", "keyboard_layout_finnish"},
        {"com.apple.keylayout.Croatian",
         "keyboard_layout_croatian_and_slovenian"},
        {"com.apple.keylayout.Czech-QWERTY", "keyboard_layout_czech"},
        {"com.apple.keylayout.Estonian", "keyboard_layout_estonian"},
        {"com.apple.keylayout.Hungarian", "keyboard_layout_hungarian"},
        {"com.apple.keylayout.Icelandic", "keyboard_layout_icelandic"},
        {"com.apple.keylayout.Brazilian", "keyboard_layout_brazilian"},
        {"com.apple.keylayout.Portuguese", "keyboard_layout_portuguese"},
        {"com.apple.keylayout.Slovak-QWERTY", "keyboard_layout_slovak"},
        {"com.apple.keylayout.Slovenian",
         "keyboard_layout_croatian_and_slovenian"},
        {"com.apple.keylayout.Turkish-QWERTY-PC", "keyboard_layout_turkish"},
        {"com.apple.keylayout.Ukrainian", "keyboard_layout_ukrainian"},
        {"com.apple.keylayout.Latvian", "keyboard_layout_latvian"},
        {"com.apple.keylayout.Persian", "keyboard_layout_persian"},
        {"com.apple.keylayout.Azeri", "keyboard_layout_azerbaijani"},
        {"com.apple.keylayout.Polish", "keyboard_layout_polish"},
};

#endif

const char* skin_keyboard_host_to_guest_layout_name(const char* name) {
    size_t i;
    if (name) {
        for (i = 0; i < sizeof(s_native_to_android_keyboard_map) /
                                sizeof(s_native_to_android_keyboard_map[0]);
             i++) {
#if defined(_WIN32)
            if (strtoi(name, NULL, 16) ==
                strtoi(s_native_to_android_keyboard_map[i].native_kb_layout,
                       NULL, 16))
#else
            if (strcmp(name,
                       s_native_to_android_keyboard_map[i].native_kb_layout) ==
                0)
#endif
            {
                return s_native_to_android_keyboard_map[i].android_kb_layout;
            }
        }
    }
    return NULL;
}

#endif /* CONFIG_HEADLESS */
