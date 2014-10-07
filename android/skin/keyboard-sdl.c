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
#include "android/utils/debug.h"

#include <SDL.h>

#define  DEBUG  1

#if DEBUG
#  define  D(...)  VERBOSE_PRINT(keys,__VA_ARGS__)
#else
#  define  D(...)  ((void)0)
#endif

// Convert an SDLK_XXX code into the corresponding Linux keycode value.
// On failure, return -1.
static int sdl_sym_to_key_code(int sym) {
#define KK(x,y)  { SDLK_ ## x, KEY_ ## y }
#define K1(x)    KK(x,x)
    static const struct {
        int sdl_sym;
        int keycode;
    } kConvert[] = {
        K1(LEFT),
        K1(RIGHT),
        K1(UP),
        K1(DOWN),
        K1(KP0),
        K1(KP1),
        K1(KP2),
        K1(KP3),
        K1(KP4),
        K1(KP5),
        K1(KP6),
        K1(KP7),
        K1(KP8),
        K1(KP9),
        KK(KP_MINUS,KPMINUS),
        KK(KP_PLUS,KPPLUS),
        KK(KP_MULTIPLY,KPASTERISK),
        KK(KP_DIVIDE,KPSLASH),
        KK(KP_EQUALS,KPEQUAL),
        KK(KP_PERIOD,KPDOT),
        KK(KP_ENTER,KPENTER),
        KK(ESCAPE,ESC),
        K1(0),
        K1(1),
        K1(2),
        K1(3),
        K1(4),
        K1(5),
        K1(6),
        K1(7),
        K1(8),
        K1(9),
        K1(MINUS),
        KK(EQUALS,EQUAL),
        K1(BACKSPACE),
        K1(HOME),
        KK(ESCAPE,ESC),
        K1(F1),
        K1(F2),
        K1(F3),
        K1(F4),
        K1(F5),
        K1(F6),
        K1(F7),
        K1(F8),
        K1(F9),
        K1(F10),
        K1(F11),
        K1(F12),
        KK(a,A),
        KK(b,B),
        KK(c,C),
        KK(d,D),
        KK(e,E),
        KK(f,F),
        KK(g,G),
        KK(h,H),
        KK(i,I),
        KK(j,J),
        KK(k,K),
        KK(l,L),
        KK(m,M),
        KK(n,N),
        KK(o,O),
        KK(p,P),
        KK(q,Q),
        KK(r,R),
        KK(s,S),
        KK(t,T),
        KK(u,U),
        KK(v,V),
        KK(w,W),
        KK(x,X),
        KK(y,Y),
        KK(z,Z),
        K1(COMMA),
        KK(PERIOD,DOT),
        K1(SPACE),
        K1(SLASH),
        KK(RETURN,ENTER),
        K1(TAB),
        KK(BACKQUOTE,GRAVE),
        KK(LEFTBRACKET,LEFTBRACE),
        KK(RIGHTBRACKET,RIGHTBRACE),
        K1(BACKSLASH),
        K1(SEMICOLON),
        KK(QUOTE,APOSTROPHE),
        KK(RSHIFT,RIGHTSHIFT),
        KK(LSHIFT,LEFTSHIFT),
        KK(RMETA,COMPOSE),
        KK(LMETA,COMPOSE),
        KK(RALT,RIGHTALT),
        KK(LALT,LEFTALT),
        KK(RCTRL,RIGHTCTRL),
        KK(LCTRL,LEFTCTRL),
        K1(NUMLOCK),
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (sym == kConvert[nn].sdl_sym) {
            return kConvert[nn].keycode;
        }
    }
    return -1;
}

static SkinKeyMod sdl_mod_to_key_mod(int sdl_mod) {
    static const struct {
        int sdl_flag;
        int mod_flag;
    } kConvert[] = {
        { KMOD_LSHIFT, kKeyModLShift },
        { KMOD_RSHIFT, kKeyModRShift },
        { KMOD_LCTRL, kKeyModLCtrl },
        { KMOD_RCTRL, kKeyModRCtrl },
        { KMOD_LALT, kKeyModLAlt },
        { KMOD_RALT, kKeyModRAlt },
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    SkinKeyMod mod = 0;
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (sdl_mod && kConvert[nn].sdl_flag) {
            mod |= kConvert[nn].mod_flag;
        }
    }
    return mod;
}

static bool skin_key_code_is_arrow(int code) {
    return code == kKeyCodeDpadLeft ||
           code == kKeyCodeDpadRight ||
           code == kKeyCodeDpadUp ||
           code == kKeyCodeDpadDown;
}

static SkinKeyCode
skin_keyboard_key_to_code(SkinKeyboard*  keyboard,
                          unsigned       sdl_sym,
                          int            sdl_mod,
                          int            down)
{
    SkinKeyCommand  command;

    SkinKeyCode code = sdl_sym_to_key_code(sdl_sym);
    SkinKeyMod  mod = sdl_mod_to_key_mod(sdl_mod);

    D("key code=%d mod=%d str=%s",
      code,
      mod,
      skin_key_pair_to_string(code, mod));

    /* first, handle the arrow keys directly */
    if (skin_key_code_is_arrow(code)) {
        code = skin_keycode_rotate(code, -keyboard->rotation);
        D("handling arrow (code=%d mod=%d)", code, mod);
        if (!keyboard->raw_keys) {
            int  doCapL, doCapR, doAltL, doAltR;

            if (!down) {
                LastKey*  k = skin_keyboard_find_last(keyboard, code);
                if (k != NULL) {
                    mod = k->mod;
                    skin_keyboard_remove_last(keyboard, code);
                }
            } else {
                skin_keyboard_add_last(keyboard, code, mod, 0);
            }

            doCapL = (mod & 0x7ff) & kKeyModLShift;
            doCapR = (mod & 0x7ff) & kKeyModRShift;
            doAltL = (mod & 0x7ff) & kKeyModLAlt;
            doAltR = (mod & 0x7ff) & kKeyModRAlt;

            if (down) {
                if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 1);
                if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 1);
                if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 1);
                if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 1);
            }
            skin_keyboard_add_key_event(keyboard, code, down);

            if (!down) {
                if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 0);
                if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 0);
                if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 0);
                if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 0);
            }
            code = 0;
        }
        return code;
    }

    /* special case for keypad keys, ignore them here if numlock is on */
    if ((sdl_mod & KMOD_NUM) != 0) {
        switch ((int)code) {
            case KEY_KP0:
            case KEY_KP1:
            case KEY_KP2:
            case KEY_KP3:
            case KEY_KP4:
            case KEY_KP5:
            case KEY_KP6:
            case KEY_KP7:
            case KEY_KP8:
            case KEY_KP9:
            case KEY_KPPLUS:
            case KEY_KPMINUS:
            case KEY_KPASTERISK:
            case KEY_KPSLASH:
            case KEY_KPEQUAL:
            case KEY_KPDOT:
            case KEY_KPENTER:
                return 0;
            default:
                ;
        }
    }

    /* now try all keyset combos */
    command = skin_keyset_get_command(keyboard->kset, code, mod);
    if (command != SKIN_KEY_COMMAND_NONE) {
        D("handling command %s from (sym=%d, mod=%d, str=%s)",
          skin_key_command_to_str(command),
          code,
          mod,
          skin_key_pair_to_string(code, mod));
        skin_keyboard_cmd(keyboard, command, down);
        return 0;
    }
    D("could not handle (code=%d, mod=%d, str=%s)", code, mod,
      skin_key_pair_to_string(code,mod));
    return -1;
}

/* this gets called only if the reverse unicode mapping didn't work
 * or wasn't used (when in raw keys mode)
 */

void
skin_keyboard_enable( SkinKeyboard*  keyboard,
                      int            enabled )
{
    keyboard->enabled = enabled;
    if (enabled) {
        SDL_EnableUNICODE(!keyboard->raw_keys);
        SDL_EnableKeyRepeat(0,0);
    }
}

void
skin_keyboard_process_event( SkinKeyboard*  kb, SDL_Event*  ev, int  down )
{
    int unicode = ev->key.keysym.unicode;
    int sdl_sym = ev->key.keysym.sym;
    int sdl_mod = ev->key.keysym.mod;

    /* ignore key events if we're not enabled */
    if (!kb->enabled) {
        printf( "ignoring key event sdl_sym=%d sdl_mod=0x%x unicode=%d\n",
                sdl_sym, sdl_mod, unicode );
        return;
    }

    /* first, try the keyboard-mode-independent keys */
    int code = skin_keyboard_key_to_code(kb, sdl_sym, sdl_mod, down);
    if (code == 0) {
        return;
    }
    if ((int)code > 0) {
        skin_keyboard_do_key_event(kb, code, down);
        skin_keyboard_flush(kb);
        return;
    }

    /* Ctrl-K is used to switch between 'unicode' and 'raw' modes */
    if (sdl_sym == SDLK_k)
    {
        int  mod2 = sdl_mod & 0x7ff;

        if ( mod2 == KMOD_LCTRL || mod2 == KMOD_RCTRL ) {
            if (down) {
                skin_keyboard_clear_last(kb);
                kb->raw_keys = !kb->raw_keys;
                SDL_EnableUNICODE(!kb->raw_keys);
                D( "switching keyboard to %s mode", kb->raw_keys ? "raw" : "unicode" );
            }
            return;
        }
    }

    code = sdl_sym_to_key_code(sdl_sym);
    int mod = sdl_mod_to_key_mod(sdl_mod);


    if (!kb->raw_keys) {
       /* ev->key.keysym.unicode is only valid on keydown events, and will be 0
        * on the corresponding keyup ones, so remember the set of last pressed key
        * syms to "undo" the job
        */
        if (!down && unicode == 0) {
            LastKey* k = skin_keyboard_find_last(kb, code);
            if (k != NULL) {
                unicode = k->unicode;
                skin_keyboard_remove_last(kb, code);
            }
        }
    }
    if (!kb->raw_keys &&
        skin_keyboard_process_unicode_event(kb, unicode, down) > 0)
    {
        if (down) {
            skin_keyboard_add_last(kb, code, mod, unicode);
        }
        skin_keyboard_flush(kb);
        return;
    }

    if ( !kb->raw_keys &&
         (code == kKeyCodeAltLeft  || code == kKeyCodeAltRight ||
          code == kKeyCodeCapLeft  || code == kKeyCodeCapRight ||
          code == kKeyCodeSym) ) {
        return;
    }

    if (code == -1) {
        D("ignoring keysym %d", sdl_sym);
    } else if (code > 0) {
        skin_keyboard_do_key_event(kb, code, down);
        skin_keyboard_flush(kb);
    }
}
