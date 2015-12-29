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

#include "android/utils/system.h"

extern "C" {
    #include "android/emulator-window.h"
    #include "ui/console.h"
}

/*

TECHNICAL NOTE:

DisplayState <--> QFrameBuffer <--> QEmulator/SDL

*/

/* QFrameBuffer producer callbacks */

/* this is called periodically by the GUI timer to check for updates
 * and poll user events. Use vga_hw_update().
 */
static void
android_display_producer_check(void *opaque)
{
    /* core: call vga_hw_update(). this will eventually
     * lead to calls to android_display_update()
     */
    (void)opaque;
    graphic_hw_update(NULL);
}

static void
android_display_producer_invalidate(void *opaque)
{
    (void)opaque;
    graphic_hw_invalidate(NULL);
}


namespace {
    struct DCLExtra : public DisplayChangeListener {
        DCLExtra() {
            memset(this, 0, sizeof(*this));
        }

        QFrameBuffer* fb;
    };
}

static DCLExtra* asDcl(DisplayChangeListener* dcl) {
    return static_cast<DCLExtra*>(dcl);
}

/* QFrameBuffer client callbacks */

/* this is called from dpy_update() each time a hardware framebuffer
 * rectangular update was detected. Send this to the QFrameBuffer.
 */
static void
android_display_update(DisplayChangeListener* dcl, int x, int y, int w, int h)
{
    QFrameBuffer* qfbuff = asDcl(dcl)->fb;
    qframebuffer_update(qfbuff, x, y, w, h);
}

static void
android_display_switch(DisplayChangeListener* dcl,
                       struct DisplaySurface* new_surface_unused)
{
    QFrameBuffer* qfbuff = asDcl(dcl)->fb;
    qframebuffer_rotate(qfbuff, 0);
}

static void
android_display_refresh(DisplayChangeListener* dcl)
{
    QFrameBuffer* qfbuff = asDcl(dcl)->fb;
    qframebuffer_poll(qfbuff);
}

static QemuConsole* find_graphic_console() {
    // find the first graphic console (Android emulator has only one usually)
    for (int i = 0; ; i++) {
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

bool android_display_init(DisplayState* ds, QFrameBuffer* qf)
{
    QemuConsole* const con = find_graphic_console();
    if (!con) {
        return false;
    }

    qframebuffer_set_producer(qf, ds,
                              android_display_producer_check,
                              android_display_producer_invalidate,
                              NULL); // detach


    /* Replace the display surface with one with the right dimensions */
    pixman_format_code_t format =
            qemu_default_pixman_format(qf->bits_per_pixel, true);

    auto surface =
            qemu_create_displaysurface_from(
                qf->width, qf->height, format, qf->pitch, (uint8_t*)qf->pixels);

    dpy_gfx_replace_surface(con, surface);

    /* Register a change listener for it */
    auto dcl = new DCLExtra();
    dcl->fb = qf;

    dclOps.dpy_name = "qemu2-glue";
    dclOps.dpy_refresh = &android_display_refresh;
    dclOps.dpy_gfx_update = &android_display_update;
    dclOps.dpy_gfx_switch = &android_display_switch;
    dcl->ops = &dclOps;
    register_displaychangelistener(dcl);

    return true;
}

extern "C"
bool sdl_display_init(DisplayState *ds, int full_screen, int no_frame)
{
    EmulatorWindow* emulator = emulator_window_get();
    SkinDisplay* disp =
            skin_layout_get_display(emulator_window_get_layout(emulator));
    int           width, height;
    char          buf[128];

    if (disp->rotation & 1) {
        width  = disp->rect.size.h;
        height = disp->rect.size.w;
    } else {
        width  = disp->rect.size.w;
        height = disp->rect.size.h;
    }
    snprintf(buf, sizeof buf, "width=%d,height=%d", width, height);

    return android_display_init(ds, qframebuffer_fifo_get());
}
