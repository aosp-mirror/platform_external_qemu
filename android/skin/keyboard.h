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
#ifndef _ANDROID_SKIN_KEYBOARD_H
#define _ANDROID_SKIN_KEYBOARD_H

#include "android/skin/charmap.h"
#include "android/skin/event.h"
#include "android/skin/image.h"  /* for SkinRotation */
#include "android/skin/keycode-buffer.h"
#include "android/skin/keyset.h"
#include "android/utils/aconfig-file.h"

typedef struct SkinKeyboard   SkinKeyboard;

typedef void (*SkinKeyCommandFunc)( void*  opaque, SkinKeyCommand  command, int  param );

typedef void (*SkinKeyEventFunc)( void*  opaque, SkinKeyCode  code, int  down );

/* If kcm_file_path is NULL, create a keyboard using the default built-in qwerty2 charmap */
extern SkinKeyboard*  skin_keyboard_create(const char* kcm_file_path,
                                           int use_raw_keys,
                                           SkinKeyCodeFlushFunc keycode_flush);

extern void           skin_keyboard_set_keyset( SkinKeyboard*  keyboard, SkinKeyset*  kset );

extern const char*    skin_keyboard_charmap_name( SkinKeyboard*  keyboard );

extern void           skin_keyboard_free( SkinKeyboard*  keyboard );

extern void           skin_keyboard_enable( SkinKeyboard*  keyboard,
                                            int            enabled );

extern void           skin_keyboard_on_command( SkinKeyboard*       keyboard,
                                                SkinKeyCommandFunc  cmd_func,
                                                void*               cmd_opaque );

extern void           skin_keyboard_set_rotation( SkinKeyboard*     keyboard,
                                                  SkinRotation      rotation );

extern SkinKeyCode    skin_keyboard_rotate_keycode( SkinKeyboard*   keyboard,
                                                    SkinKeyCode  keycode );

extern void           skin_keyboard_on_key_press( SkinKeyboard*     keyboard,
                                                  SkinKeyEventFunc  press_func,
                                                  void*             press_opaque );

extern void           skin_keyboard_process_event( SkinKeyboard*  keyboard, SkinEvent*  ev, int  down );
extern int            skin_keyboard_process_unicode_event( SkinKeyboard*  kb,  unsigned int  unicode, int  down );

extern void           skin_keyboard_add_key_event( SkinKeyboard*  k, unsigned code, unsigned  down );
extern void           skin_keyboard_flush( SkinKeyboard*  kb );

#endif /* _ANDROID_SKIN_KEYBOARD_H */

