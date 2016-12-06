/* Copyright (C) 2010 The Android Open Source Project
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

/* Initialization of the Android-specific DisplayState.
 * Read docs/DISPLAY-STATE.TXT to understand what this
 * is supposed to do.
 */
#include "android-qemu2-glue/display.h"

#include "android/emulator-window.h"

extern "C" {
#include "qemu/osdep.h"
#include "ui/console.h"
}

namespace {

struct DCLExtra : public DisplayChangeListener {
    DCLExtra() { memset(this, 0, sizeof(*this)); }

    QFrameBuffer* fb;
};

}

static DCLExtra* asDcl(DisplayChangeListener* dcl) {
    return static_cast<DCLExtra*>(dcl);
}

/*

TECHNICAL NOTE:

DisplayState <--> QFrameBuffer <--> QEmulator/SDL

*/

/* QFrameBuffer producer callbacks */

/* this is called periodically by the GUI timer to check for updates
 * and poll user events. Use vga_hw_update().
 */
static void android_display_producer_check(void* opaque) {
    /* core: call vga_hw_update(). this will eventually
     * lead to calls to android_display_update()
     */
    (void)opaque;
    graphic_hw_update(NULL);
}

static void android_display_producer_invalidate(void* opaque) {
    (void)opaque;
    graphic_hw_invalidate(NULL);
}

static void android_display_producer_detach(void* opaque) {
    // the framebuffer is being deleted, clean it up in the DCL as well
    if (const auto dcl = static_cast<DCLExtra*>(opaque)) {
        dcl->fb = nullptr;
    }
}

/* QFrameBuffer client callbacks */

/* this is called from dpy_update() each time a hardware framebuffer
 * rectangular update was detected. Send this to the QFrameBuffer.
 */
static void android_display_update(DisplayChangeListener* dcl,
                                   int x,
                                   int y,
                                   int w,
                                   int h) {
    if (QFrameBuffer* qfbuff = asDcl(dcl)->fb) {
        qframebuffer_update(qfbuff, x, y, w, h);
    }
}

static void android_display_switch(DisplayChangeListener* dcl,
                                   DisplaySurface* new_surface_unused) {
    if (QFrameBuffer* qfbuff = asDcl(dcl)->fb) {
        qframebuffer_rotate(qfbuff, 0);
    }
}

static void android_display_refresh(DisplayChangeListener* dcl) {
    if (QFrameBuffer* qfbuff = asDcl(dcl)->fb) {
        qframebuffer_poll(qfbuff);
    }
}

static QemuConsole* find_graphic_console() {
    // find the first graphic console (Android emulator has only one usually)
    for (int i = 0;; i++) {
        QemuConsole* const c = qemu_console_lookup_by_index(i);
        if (!c) {
            break;
        }
        if (qemu_console_is_graphic(c)) {
            return c;
        }
    }

    return NULL;
}

static DisplayChangeListenerOps dclOps = {};

bool android_display_init(DisplayState* ds, QFrameBuffer* qf) {
    QemuConsole* const con = find_graphic_console();
    if (!con) {
        return false;
    }

    const auto dcl = new DCLExtra();

    qframebuffer_set_producer(qf, dcl,
                              android_display_producer_check,
                              android_display_producer_invalidate,
                              android_display_producer_detach);

    /* Replace the display surface with one with the right dimensions */
    pixman_format_code_t format =
            qemu_default_pixman_format(qf->bits_per_pixel, true);

    auto surface = qemu_create_displaysurface_from(
            qf->width, qf->height, format, qf->pitch, (uint8_t*)qf->pixels);

    dpy_gfx_replace_surface(con, surface);

    /* Register a change listener for it */
    dcl->fb = qf;

    dclOps.dpy_name = "qemu2-glue";
    dclOps.dpy_refresh = &android_display_refresh;
    dclOps.dpy_gfx_update = &android_display_update;
    dclOps.dpy_gfx_switch = &android_display_switch;
    dcl->ops = &dclOps;
    register_displaychangelistener(dcl);

    return true;
}

extern "C" void sdl_display_early_init(int opengl) {
    (void)opengl;
}

extern "C" bool sdl_display_init(DisplayState* ds,
                                 int full_screen,
                                 int no_frame) {
    (void)full_screen;
    (void)no_frame;

    EmulatorWindow* const emulator = emulator_window_get();
    if (emulator->opts->no_window) {
        return true;
    }

    return android_display_init(ds, qframebuffer_fifo_get());
}
