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
#ifndef ANDROID_SKIN_KEYBOARD_INT_H
#define ANDROID_SKIN_KEYBOARD_INT_H

#include "android/skin/keyboard.h"

// Internal implementation details for SkinKeyboard
/** LAST PRESSED KEYS
 ** a small buffer of last pressed keys, this is used to properly
 ** implement the Unicode keyboard mode (SDL key up event always have
 ** their .unicode field set to 0
 **/
typedef struct {
    int  unicode;  /* Unicode of last pressed key        */
    int  sym;      /* SDL key symbol value (e.g. SDLK_a) */
    int  mod;      /* SDL key modifier value             */
} LastKey;

#define  MAX_LAST_KEYS  16

struct SkinKeyboard {
    const SkinCharmap*  charmap;
    SkinKeyset*         kset;
    char                enabled;
    char                raw_keys;
    char                last_count;

    SkinRotation        rotation;

    SkinKeyCommandFunc  command_func;
    void*               command_opaque;
    SkinKeyEventFunc    press_func;
    void*               press_opaque;

    LastKey             last_keys[ MAX_LAST_KEYS ];

    SkinKeycodeBuffer   keycodes[1];
};


extern LastKey* skin_keyboard_find_last(SkinKeyboard*  keyboard, int sym);

extern void skin_keyboard_add_last(SkinKeyboard*  keyboard,
                                   int            sym,
                                   int            mod,
                                   int            unicode);

extern void skin_keyboard_remove_last(SkinKeyboard* keyboard, int sym);

extern void skin_keyboard_clear_last(SkinKeyboard*  keyboard);

extern void skin_keyboard_do_key_event(SkinKeyboard* kb,
                                       SkinKeyCode code,
                                       int down);

extern void skin_keyboard_cmd(SkinKeyboard*   keyboard,
                              SkinKeyCommand  command,
                              int             param);

#endif  // ANDROID_SKIN_KEYBOARD_INT_H
