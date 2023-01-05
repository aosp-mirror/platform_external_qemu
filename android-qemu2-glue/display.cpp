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
#include "host-common/hw-config.h"
#include "android/console.h"
#include "android/utils/debug.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu/module.h"
#include "sysemu/sysemu.h"
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

static pixman_image_t* get_pixman_image_from_qframebuffer(QFrameBuffer* qf) {
    pixman_format_code_t format =
            qemu_default_pixman_format(qf->bits_per_pixel, true);
    return pixman_image_create_bits(format, qf->width, qf->height,
                                    (uint32_t*)qf->pixels, qf->pitch);
}

static void android_display_switch(DisplayChangeListener* dcl,
                                   DisplaySurface* new_surface) {
    if (QFrameBuffer* qfbuff = asDcl(dcl)->fb) {
        if (getConsoleAgents()->settings->hw()->hw_arc) {
            static pixman_image_t * qfb_image =
                    get_pixman_image_from_qframebuffer(qfbuff);
            pixman_image_composite(PIXMAN_OP_OVER,
                                   new_surface->image, NULL,
                                   qfb_image,
                                   0, 0, 0, 0, 0, 0,
                                   qfbuff->width, qfbuff->height);
        }
        qframebuffer_rotate(qfbuff, 0);
    }
}

static void android_display_refresh(DisplayChangeListener* dcl) {
    if (QFrameBuffer* qfbuff = asDcl(dcl)->fb) {
        qframebuffer_poll(qfbuff);
    }
}

extern void * android_gl_create_context(DisplayChangeListener*, QEMUGLParams *);
extern void android_gl_destroy_context(DisplayChangeListener*, void *);
extern int android_gl_make_context_current(DisplayChangeListener*, void *);
extern void android_gl_scanout_texture(DisplayChangeListener*, uint32_t, bool,
                                       uint32_t, uint32_t, uint32_t, uint32_t,
                                       uint32_t, uint32_t);
extern void android_gl_scanout_flush(DisplayChangeListener *, uint32_t,
                                     uint32_t, uint32_t, uint32_t);

static void android_gl_scanout_disable(DisplayChangeListener *dcl) {
    dinfo("stub %s", __func__);
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

static int last_graphic_console_index() {
    int last = -1;
    for (int i = 0;; i++) {
        QemuConsole* const c = qemu_console_lookup_by_index(i);
        if (!c) {
            break;
        }
        if (qemu_console_is_graphic(c)) {
            last = i;
        }
    }
    return last;
}

static DisplayChangeListenerOps dclOps = {};

void android_display_init_no_window(QFrameBuffer* qf) {
    // Only need to attach the check and invalidate callbacks in no-window mode
    // to request for a refresh of the framebuffer.
    qframebuffer_set_producer(qf, nullptr, android_display_producer_check,
                              android_display_producer_invalidate, nullptr);
}

bool android_display_init(DisplayState* ds, QFrameBuffer* qf) {
    QemuConsole* con = find_graphic_console();
    if (!con) {
        return false;
    }
    if (getConsoleAgents()->settings->hw()->hw_arc) {
        /* We don't use goldfish_fb in cros now. so
         * just pick up last graphic console */
        int index = last_graphic_console_index();
        if (index < 0) {
            return false;
        }
        console_select(index);
        con = qemu_console_lookup_by_index(index);
        QemuUIInfo info = {
            0, 0,
            (uint32_t)qf->width,
            (uint32_t)qf->height,
            0,
        };
        dpy_set_ui_info(con, &info);
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

    if (getConsoleAgents()->settings->hw()->hw_arc) {
        dclOps.dpy_gl_ctx_create       = &android_gl_create_context;
        dclOps.dpy_gl_ctx_destroy      = &android_gl_destroy_context;
        dclOps.dpy_gl_ctx_make_current = &android_gl_make_context_current;
        dclOps.dpy_gl_scanout_disable  = &android_gl_scanout_disable;
        dclOps.dpy_gl_scanout_texture  = &android_gl_scanout_texture;
        dclOps.dpy_gl_update           = &android_gl_scanout_flush;

        dcl->con = con;
    }

    dcl->ops = &dclOps;
    register_displaychangelistener(dcl);

    return true;
}

#if defined(__APPLE__) && defined(__arm64)
extern "C" void android_sdl_display_early_init(DisplayOptions* opts) {
    if (opts->gl && getConsoleAgents()->settings->hw()->hw_arc) {
        display_opengl = 1;
    }
}

extern "C" int android_sdl_display_init(DisplayState* ds, DisplayOptions* opts) {
    (void)opts;

    EmulatorWindow* const emulator = emulator_window_get();
    if (emulator->opts->no_window) {
        return true;
    }

    return android_display_init(ds, qframebuffer_fifo_get());
}

static QemuDisplay qemu_display_android = {
    .type       = DISPLAY_TYPE_SDL,
    .early_init = android_sdl_display_early_init,
    .init       = android_sdl_display_init,
};

#else

extern "C" void sdl_display_early_init(DisplayOptions* opts) {
    if (opts->gl && getConsoleAgents()->settings->hw()->hw_arc) {
        display_opengl = 1;
    }
}

extern "C" int sdl_display_init(DisplayState* ds, DisplayOptions* opts) {
    (void)opts;

    EmulatorWindow* const emulator = emulator_window_get();
    if (emulator->opts->no_window) {
        return true;
    }

    return android_display_init(ds, qframebuffer_fifo_get());
}

static QemuDisplay qemu_display_android = {
    .type       = DISPLAY_TYPE_SDL,
    .early_init = sdl_display_early_init,
    .init       = sdl_display_init,
};

#endif

static void register_android_ui(void)
{
    qemu_display_register(&qemu_display_android);
}

type_init(register_android_ui);

