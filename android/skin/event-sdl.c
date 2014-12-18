/* Copyright (C) 2014 The Android Open Source Project
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
#include "android/skin/event.h"

#include "android/skin/keycode.h"
#include "android/utils/utf8_utils.h"

#include <SDL.h>

#include <stdbool.h>

/** LAST PRESSED KEYS
 ** a small buffer of last pressed keys, this is used to properly
 ** implement the Unicode keyboard mode (SDL key up event always have
 ** their .unicode field set to 0).
 **/

typedef struct {
    int  unicode;  /* Unicode of last pressed key        */
    int  sym;      /* SDL key symbol value (e.g. SDLK_a) */
    int  mod;      /* SDL key modifier value             */
} LastKey;

#define  MAX_LAST_KEYS  16

typedef struct {
    int count;
    LastKey keys[ MAX_LAST_KEYS ];
} LastKeys;

#define LAST_KEYS_INIT  { 0, }

static void last_keys_clear(LastKeys* l) {
    l->count = 0;
}

static LastKey* last_keys_find(LastKeys* l, int sym) {
    LastKey*  k = l->keys;
    LastKey*  end = k + l->count;

    for ( ; k < end; k++ ) {
        if (k->sym == sym)
            return k;
    }
    return NULL;
}

static void last_keys_add(LastKeys* l, int sym, int mod, int unicode) {
    LastKey* k = l->keys + l->count;

    if (l->count < MAX_LAST_KEYS) {
        k->sym     = sym;
        k->mod     = mod;
        k->unicode = unicode;
        l->count += 1;
    }
}

static void last_keys_remove(LastKeys* l, int sym) {
    LastKey* k = l->keys;
    LastKey* end = k + l->count;

    for ( ; k < end; k++ ) {
        if (k->sym == sym) {
           /* we don't need a sorted array, so place the last
            * element in place at the position of the removed
            * one... */
            k[0] = end[-1];
            l->count -= 1;
            break;
        }
    }
}

static LastKeys sLastKeys = LAST_KEYS_INIT;

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
        if (sdl_mod & kConvert[nn].sdl_flag) {
            mod |= kConvert[nn].mod_flag;
        }
    }
    return mod;
}

static int sdl_mouse_button_to_button(int sdl_button) {
    switch (sdl_button) {
        case 0: return 0;
        case SDL_BUTTON_LEFT: return kMouseButtonLeft;
        case SDL_BUTTON_RIGHT: return kMouseButtonRight;
        case SDL_BUTTON_MIDDLE: return kMouseButtonCenter;
        // scroll-wheel
        case 4: return kMouseButtonScrollUp;
        case 5: return kMouseButtonScrollDown;
        default: return -1;
    }
}

// Return true iff this event corresponds to the standard window system
// exit key combination (e.g. Alt-F4 on Windows, Command-Q on Apple).
static bool is_winsys_exit(SDL_Event* ev) {
#ifdef _WIN32
    /* special code to deal with Alt-F4 properly */
    return (ev->key.keysym.sym == SDLK_F4 && ev->key.keysym.mod & KMOD_ALT);
#elif defined(__APPLE__)
    return (ev->key.keysym.sym == SDLK_q && ev->key.keysym.mod & KMOD_META);
#else
    return false;
#endif
}

static bool prepare_text_input_event(SkinEvent* event,
                                     uint32_t unicode, 
                                     bool down) {
    event->type = kEventTextInput;
    // Encode the Unicode value as a 0-terminated UTF-8 value.
    int len = android_utf8_encode(unicode,
                                  event->u.text.text,
                                  sizeof(event->u.text.text));
    if (len < 0) {
        return false;
    }
    event->u.text.down = down;
    event->u.text.text[len] = '\0';
    return true;
}

bool skin_event_poll(SkinEvent* event) {
    SDL_Event ev;

    static SkinEvent pending_event;
    static bool pending = false;

    // We need to buffer at most one event to support text input with SDL 1.x
    if (pending) {
        pending = false;
        event[0] = pending_event;
        return true;
    }

    for (;;) {
        if (!SDL_PollEvent(&ev)) {
            return false;
        }

        switch (ev.type) {
            case SDL_VIDEOEXPOSE:
                event->type = kEventVideoExpose;
                return true;

            case SDL_KEYDOWN:
                if (is_winsys_exit(&ev)) {
                    event->type = kEventQuit;
                    return true;
                }
                // fall-through.

            case SDL_KEYUP: {
                bool down = (ev.type == SDL_KEYDOWN);
                event->type = down ? kEventKeyDown : kEventKeyUp;

                int sdl_sym = ev.key.keysym.sym;
                int sdl_mod = ev.key.keysym.mod;
                int unicode = ev.key.keysym.unicode;

                event->u.key.keycode = sdl_sym_to_key_code(sdl_sym);
                event->u.key.mod = sdl_mod_to_key_mod(sdl_mod);

                // Prepare a pending text input event for the next call.

                // ev.key.keysym.unicode is only valid on keydown events.
                // and will be 0 on the corresponding keyup ones, so
                // remember the set of last pressed key syms to "undo" the
                // job.
                LastKeys* last_keys = &sLastKeys;
                if (ev.type == SDL_KEYUP) {
                    LastKey* k = last_keys_find(last_keys, sdl_sym);
                    if (k != NULL) {
                        if (!unicode) {
                            unicode = k->unicode;
                        }
                        if (!sdl_mod) {
                            event->u.key.mod = sdl_mod_to_key_mod(sdl_mod);
                        }
                        last_keys_remove(last_keys, sdl_sym);
                    }
                } else if (ev.type == SDL_KEYDOWN && unicode) {
                    last_keys_add(last_keys, sdl_sym, sdl_mod, unicode);
                }

                if (unicode && down) {
                    // The unicode value is added to a pending event
                    // unless it is invalid.
                    if (prepare_text_input_event(
                            &pending_event,
                            unicode,
                            down)) {
                        pending = true;
                    }
                }

                return true;
                }

            case SDL_MOUSEMOTION:
                event->type = kEventMouseMotion;
                event->u.mouse.x = ev.button.x;
                event->u.mouse.y = ev.button.y;
                event->u.mouse.xrel = ev.motion.xrel;
                event->u.mouse.yrel = ev.motion.yrel;
                event->u.mouse.button =
                        sdl_mouse_button_to_button(ev.button.button);
                if (event->u.mouse.button < 0) {
                    break;
                }
                return true;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                event->type = (ev.type == SDL_MOUSEBUTTONDOWN) ?
                        kEventMouseButtonDown :
                        kEventMouseButtonUp;
                event->u.mouse.x = ev.button.x;
                event->u.mouse.y = ev.button.y;
                event->u.mouse.button =
                        sdl_mouse_button_to_button(ev.button.button);
                if (event->u.mouse.button < 0) {
                    break;
                }
                return true;

            case SDL_QUIT:
                event->type = kEventQuit;
                return true;

            default:
                // Ignore other events.
                ;
        }
    }
}

void skin_event_enable_unicode(bool enabled) {
    LastKeys* last_keys = &sLastKeys;

    last_keys_clear(last_keys);
    SDL_EnableUNICODE(enabled);
}
