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
#include "android/display.h"

#include "android/emulator-window.h"
#include "android/utils/system.h"

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
    vga_hw_update();
}

static void
android_display_producer_invalidate(void *opaque)
{
    (void)opaque;
    vga_hw_invalidate();
}

/* QFrameBuffer client callbacks */

/* this is called from dpy_update() each time a hardware framebuffer
 * rectangular update was detected. Send this to the QFrameBuffer.
 */
static void
android_display_update(DisplayState *ds, int x, int y, int w, int h)
{
    QFrameBuffer* qfbuff = ds->opaque;
    qframebuffer_update(qfbuff, x, y, w, h);
}

static void
android_display_resize(DisplayState *ds)
{
    QFrameBuffer* qfbuff = ds->opaque;
    qframebuffer_rotate(qfbuff, 0);
}

static void
android_display_refresh(DisplayState *ds)
{
    QFrameBuffer* qfbuff = ds->opaque;
    qframebuffer_poll(qfbuff);
}


void android_display_init(DisplayState* ds, QFrameBuffer* qf)
{
    DisplayChangeListener* dcl;

    qframebuffer_set_producer(qf, ds,
                              android_display_producer_check,
                              android_display_producer_invalidate,
                              NULL); // detach

    /* Replace the display surface with one with the right dimensions */
    qemu_free_displaysurface(ds);
    ds->opaque    = qf;
    ds->surface   = qemu_create_displaysurface_from(qf->width,
                                                    qf->height,
                                                    qf->bits_per_pixel,
                                                    qf->pitch,
                                                    qf->pixels);

    /* Register a change listener for it */
    ANEW0(dcl);
    dcl->dpy_update      = android_display_update;
    dcl->dpy_resize      = android_display_resize;
    dcl->dpy_refresh     = android_display_refresh;
    dcl->dpy_text_cursor = NULL;

    register_displaychangelistener(ds, dcl);
}

void sdl_display_init(DisplayState *ds, int full_screen, int  no_frame)
{
    EmulatorWindow*    emulator = emulator_window_get();
    SkinDisplay*  disp =
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
    android_display_init(ds, qframebuffer_fifo_get());
}
