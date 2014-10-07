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
#include "android/skin/keyboard-int.h"

#include "android/skin/charmap.h"
#include "android/skin/keycode.h"
#include "android/skin/keycode-buffer.h"
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "android/utils/system.h"

#define  DEBUG  1

#if DEBUG
#  define  D(...)  VERBOSE_PRINT(keys,__VA_ARGS__)
#else
#  define  D(...)  ((void)0)
#endif

#define DEFAULT_ANDROID_CHARMAP  "qwerty2"

void
skin_keyboard_set_keyset( SkinKeyboard*  keyboard, SkinKeyset*  kset )
{
    if (kset == NULL)
        return;
    if (keyboard->kset && keyboard->kset != skin_keyset_get_default()) {
        skin_keyset_free(keyboard->kset);
    }
    keyboard->kset = kset;
}


void
skin_keyboard_set_rotation( SkinKeyboard*     keyboard,
                            SkinRotation      rotation )
{
    keyboard->rotation = (rotation & 3);
}

void
skin_keyboard_on_command( SkinKeyboard*  keyboard, SkinKeyCommandFunc  cmd_func, void*  cmd_opaque )
{
    keyboard->command_func   = cmd_func;
    keyboard->command_opaque = cmd_opaque;
}

void
skin_keyboard_on_key_press( SkinKeyboard*  keyboard, SkinKeyEventFunc  press_func, void*  press_opaque )
{
    keyboard->press_func   = press_func;
    keyboard->press_opaque = press_opaque;
}

void
skin_keyboard_add_key_event( SkinKeyboard*  kb,
                             unsigned       code,
                             unsigned       down )
{
    skin_keycodes_buffer_add(kb->keycodes, code, down);
}


void
skin_keyboard_flush( SkinKeyboard*  kb )
{
    skin_keycodes_buffer_flush(kb->keycodes);
}


void
skin_keyboard_cmd( SkinKeyboard*   keyboard,
                   SkinKeyCommand  command,
                   int             param )
{
    if (keyboard->command_func) {
        keyboard->command_func( keyboard->command_opaque, command, param );
    }
}


LastKey*
skin_keyboard_find_last( SkinKeyboard*  keyboard,
                         int            sym )
{
    LastKey*  k   = keyboard->last_keys;
    LastKey*  end = k + keyboard->last_count;

    for ( ; k < end; k++ ) {
        if (k->sym == sym)
            return k;
    }
    return NULL;
}

void
skin_keyboard_add_last( SkinKeyboard*  keyboard,
                        int            sym,
                        int            mod,
                        int            unicode )
{
    LastKey*  k = keyboard->last_keys + keyboard->last_count;

    if (keyboard->last_count < MAX_LAST_KEYS) {
        k->sym     = sym;
        k->mod     = mod;
        k->unicode = unicode;

        keyboard->last_count += 1;
    }
}

void
skin_keyboard_remove_last( SkinKeyboard*  keyboard,
                           int            sym )
{
    LastKey*  k   = keyboard->last_keys;
    LastKey*  end = k + keyboard->last_count;

    for ( ; k < end; k++ ) {
        if (k->sym == sym) {
           /* we don't need a sorted array, so place the last
            * element in place at the position of the removed
            * one... */
            k[0] = end[-1];
            keyboard->last_count -= 1;
            break;
        }
    }
}

void
skin_keyboard_clear_last( SkinKeyboard*  keyboard )
{
    keyboard->last_count = 0;
}

void
skin_keyboard_do_key_event( SkinKeyboard*   kb,
                            SkinKeyCode  code,
                            int             down )
{
    if (kb->press_func) {
        kb->press_func( kb->press_opaque, code, down );
    }
    skin_keyboard_add_key_event(kb, code, down);
}


int
skin_keyboard_process_unicode_event( SkinKeyboard*  kb,  unsigned int  unicode, int  down )
{
    return skin_charmap_reverse_map_unicode(kb->charmap, unicode, down,
                                            kb->keycodes);
}


static SkinKeyboard*
skin_keyboard_create_from_charmap_name(const char* charmap_name,
                                       int  use_raw_keys,
                                       SkinKeyCodeFlushFunc keycode_flush)
{
    SkinKeyboard*  kb;

    ANEW0(kb);

    kb->charmap = skin_charmap_get_by_name(charmap_name);
    if (!kb->charmap) {
        // Charmap name was not found. Default to "qwerty2" */
        kb->charmap = skin_charmap_get_by_name(DEFAULT_ANDROID_CHARMAP);
        fprintf(stderr, "### warning, skin requires unknown '%s' charmap, reverting to '%s'\n",
                charmap_name, kb->charmap->name );
    }
    kb->raw_keys = use_raw_keys;
    kb->enabled  = 0;

    /* add default keyset */
    if (skin_keyset_get_default()) {
        kb->kset = skin_keyset_get_default();
    } else {
        kb->kset = skin_keyset_new_from_text(
                skin_keyset_get_default_text());
    }
    skin_keycodes_buffer_init(kb->keycodes, keycode_flush);
    return kb;
}

SkinKeyboard*
skin_keyboard_create(const char* kcm_file_path,
                     int use_raw_keys,
                     SkinKeyCodeFlushFunc keycode_flush)
{
    const char* charmap_name = DEFAULT_ANDROID_CHARMAP;
    char        cmap_buff[SKIN_CHARMAP_NAME_SIZE];

    if (kcm_file_path != NULL) {
        kcm_extract_charmap_name(kcm_file_path, cmap_buff, sizeof cmap_buff);
        charmap_name = cmap_buff;
    }
    return skin_keyboard_create_from_charmap_name(
            charmap_name, use_raw_keys, keycode_flush);
}

void
skin_keyboard_free( SkinKeyboard*  keyboard )
{
    if (keyboard) {
        AFREE(keyboard);
    }
}
