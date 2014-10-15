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

#include "android/skin/linux_keycodes.h"

#include <SDL.h>

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEBUG 0
#if DEBUG
#  define D(...) printf(__VA_ARGS__)
#else
#  define D(...) ((void)0)
#endif

// Returns true if this is a (sym,mod) pair that corresponds to one of
// the non-Unicode keys. The intended goal is for the caller to ignore
// non-special keys because they will generate a TEXTINPUT event too.
//
// Note that SDL2 uses direct Unicode values for keycodes that match the
// correspond character / glyph. For others, the SDLK_SCANCODE_MASK bitmask
// is or-ed (see SDL_keycode.h).
static bool sdl_is_special_keypair(int sdl_sym, int sdl_mod) {
    // Ctrl-something is always a special key combination
    // Same with the GUI key.
    if (sdl_mod & (KMOD_CTRL | KMOD_GUI)) {
        return true;
    }
    // Alt-something will always generate a TEXTINPUT so it needs to be
    // treated as non-special.
    if (sdl_mod & KMOD_ALT) {
        return false;
    }
    if ((sdl_sym & SDLK_SCANCODE_MASK) != 0) {
        // This is a non-Unicode key, so it's special.
        return true;
    }
    if (sdl_sym < 32) {
        // TAB, ENTER, ESC, etc, are all special.
        return true;
    }
    return false;
}

static bool sdl_is_keypad_keycode(int keycode) {
    static int kp_codes[] = {
        SDLK_KP_DIVIDE,
        SDLK_KP_MULTIPLY,
        SDLK_KP_MINUS,
        SDLK_KP_PLUS,
        SDLK_KP_ENTER,
        SDLK_KP_1,
        SDLK_KP_2,
        SDLK_KP_3,
        SDLK_KP_4,
        SDLK_KP_5,
        SDLK_KP_6,
        SDLK_KP_7,
        SDLK_KP_8,
        SDLK_KP_9,
        SDLK_KP_0,
        SDLK_KP_PERIOD,
    };
    static const size_t kp_codes_size = sizeof(kp_codes) / sizeof(kp_codes[0]);
    size_t n;
    for (n = 0; n < kp_codes_size; ++n) {
        if (keycode == kp_codes[n]) {
            return true;
        }
    }
    return false;
}

// Convert an SDL_SCANCODE_XXX code into the corresponding Linux keycode value.
// On failure, return -1.
static int sdl_scancode_to_key_code(int scancode) {
#define KK(x,y)  { SDL_SCANCODE_ ## x, KEY_ ## y }
#define K1(x)    KK(x,x)
    static const struct {
        int scancode;
        int keycode;
    } kConvert[] = {
        K1(LEFT),
        K1(RIGHT),
        K1(UP),
        K1(DOWN),
        KK(KP_0, KP0),
        KK(KP_1, KP1),
        KK(KP_2, KP2),
        KK(KP_3, KP3),
        KK(KP_4, KP4),
        KK(KP_5, KP5),
        KK(KP_6, KP6),
        KK(KP_7, KP7),
        KK(KP_8, KP8),
        KK(KP_9, KP9),
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
        K1(A),
        K1(B),
        K1(C),
        K1(D),
        K1(E),
        K1(F),
        K1(G),
        K1(H),
        K1(I),
        K1(J),
        K1(K),
        K1(L),
        K1(M),
        K1(N),
        K1(O),
        K1(P),
        K1(Q),
        K1(R),
        K1(S),
        K1(T),
        K1(U),
        K1(V),
        K1(W),
        K1(X),
        K1(Y),
        K1(Z),
        K1(COMMA),
        KK(PERIOD,DOT),
        K1(SPACE),
        K1(SLASH),
        KK(RETURN,ENTER),
        K1(TAB),
        K1(GRAVE),
        KK(LEFTBRACKET,LEFTBRACE),
        KK(RIGHTBRACKET,RIGHTBRACE),
        K1(BACKSLASH),
        K1(SEMICOLON),
        K1(APOSTROPHE),
        KK(RSHIFT,RIGHTSHIFT),
        KK(LSHIFT,LEFTSHIFT),
        KK(RGUI,COMPOSE),
        KK(LGUI,COMPOSE),
        KK(RALT,RIGHTALT),
        KK(LALT,LEFTALT),
        KK(RCTRL,RIGHTCTRL),
        KK(LCTRL,LEFTCTRL),
        KK(NUMLOCKCLEAR, NUMLOCK),
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (scancode == kConvert[nn].scancode) {
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
        { KMOD_NUM, kKeyModNumLock },
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
    return (ev->key.keysym.sym == SDL_SCANCODE_F4 && ev->key.keysym.mod & KMOD_ALT);
#elif defined(__APPLE__)
    /* Not sure if Command-Q is used on all OS X locales. */
    return (ev->key.keysym.sym == SDL_SCANCODE_Q && ev->key.keysym.mod & KMOD_GUI);
#else
    return false;
#endif
}

bool skin_event_poll(SkinEvent* event) {
    SDL_Event ev;

    // SDL2 is very confused about the state of NumLock, it always assumes that
    // it is not active when the program starts, then detects actual NumLock
    // keypresses to toggle its internal state.
    //
    // Consider the following sequence of key presses:
    //
    //     a) Keypad 7
    //     b) NumLock
    //     c) Keypad 7
    //
    //  1) If the program starts when NumLock is not active, the value of the
    //     KMOD_NUM bit flag is consistent with the use of the NumLock key,
    //     i.e. the sequence above generates:
    //
    //       1 KEYDOWN event for Keypad_7 (KMOD_NUM is _unset_)
    //       1 KEYUP event for Keypad_7 (KMOD_NUM is _unset_)
    //
    //       1 KEYDOWN event for NUMLOCKCLEAR (with KMOD_NUM _unset_)
    //       1 KEYUP event for NUMLOCKCLEAR (with KMOD_NUM set)
    //
    //       1 KEYDOWN event for Keypad_7 (KMOD_NUM is set)
    //       1 TEXTINPUT event for '7'
    //       1 KEYUP event for Keypad_7 (KMOD_NUM is set)
    //
    //   2) If the program starts when Numlock is active, things are
    //      unfortunately different:
    //
    //        1 KEYDOWN event for Keypad_7  (KMOD_NUM is _unset_)
    //        1 TEXTINPUT event for '7'
    //        1 KEYUP event for Keypad_7  (KMOD_NUM is _unset_)
    //
    //        1 KEYDOWN event for NUMLOCKCLEAR (KMOD_NUM is _unset_)
    //        1 KEYUP event for NUMLOCKCLEAR (KMOD_NUM is set)
    //
    //        1 KEYDOWN event for Keypad_7  (KMOD_NUM is set)
    //        1 KEYUP event for Keypad_7 (KMOD_NUM is set)
    //
    // Notice that in both cases, the first KEYDOWN are identical and have
    // KMOD_NUM _unset_.
    //
    // To work around this problem, we need to detect situation 2, and "fix"
    // The KMOD_NUM state.
    static bool keypad_pressed_without_KMOD_NUM = false;

    for (;;) {
        if (!SDL_PollEvent(&ev)) {
            return false;
        }

        switch (ev.type) {
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    event->type = kEventVideoExpose;
                    return true;
                }
                break;

            case SDL_KEYDOWN:
                if (is_winsys_exit(&ev)) {
                    event->type = kEventQuit;
                    return true;
                }
                keypad_pressed_without_KMOD_NUM =
                        sdl_is_keypad_keycode(ev.key.keysym.sym) &&
                        !(ev.key.keysym.mod & KMOD_NUM);
                // fall-through.

            case SDL_KEYUP: {
                bool down = (ev.type == SDL_KEYDOWN);
                event->type = down ? kEventKeyDown : kEventKeyUp;

                int sdl_scancode = ev.key.keysym.scancode;
                int sdl_sym = ev.key.keysym.sym;
                int sdl_mod = ev.key.keysym.mod;

                if (!sdl_is_special_keypair(sdl_sym, sdl_mod)) {
                    // Ignore this key event because a TEXTINPUT event
                    // will also be generated for it.
                    D("IGNORING KEY%s: sdl_sym=0x%x sdl_mod=0x%x keycode=0x%x scancode=0x%x mod=0x%x\n",
                    down ? "DOWN" : "UP",
                    sdl_sym,
                    sdl_scancode,
                    sdl_mod,
                    event->u.key.keycode,
                    event->u.key.mod);
                    break;
                }

                event->u.key.keycode = sdl_scancode_to_key_code(sdl_scancode);
                event->u.key.mod = sdl_mod_to_key_mod(sdl_mod);

                D("KEY%s: sdl_sym=0x%x sdl_mod=0x%x keycode=0x%x scancode=0x%x mod=0x%x\n",
                  down ? "DOWN" : "UP",
                  sdl_sym,
                  sdl_scancode,
                  sdl_mod,
                  event->u.key.keycode,
                  event->u.key.mod);

                if (ev.type == SDL_KEYUP) {
                    keypad_pressed_without_KMOD_NUM = false;
                }

                return true;
                }

            case SDL_TEXTINPUT:
                // Try to ignore keypad presses.
                if (keypad_pressed_without_KMOD_NUM) {
                    // Need to fix KMOD_NUM state here.
                    SDL_Keymod mod = SDL_GetModState();
                    if (!(mod & KMOD_NUM)) {
                        mod |= KMOD_NUM;
                        SDL_SetModState(mod);
                        D("Detected NUMLOCK is active - toggling KMOD_NUM flag");
                        // Break here to ignore this keypress.
                        break;
                    }
                }

                event->type = kEventTextInput;
                event->u.text.down = true;
                snprintf((char*)event->u.text.text,
                         sizeof(event->u.text.text),
                         "%s",
                         ev.text.text);

                D("TEXTINPUT text=[%s]\n", ev.text.text);
                return true;

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
    // nothing to do here.
}
