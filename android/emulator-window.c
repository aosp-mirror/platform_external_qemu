/* Copyright (C) 2006-2010 The Android Open Source Project
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

#include "android/emulator-window.h"

#include "android/android.h"
#include "android/framebuffer.h"
#include "android/globals.h"
#include "android/gpu_frame.h"
#include "android/hw-control.h"
#include "android/hw-sensors.h"
#include "android/looper.h"
#include "android/opengles.h"
#include "android/skin/keycode.h"
#include "android/skin/winsys.h"
#include "android/user-events.h"
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "telephony/modem_driver.h"

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

static double get_default_scale( AndroidOptions*  opts );

/* EmulatorWindow structure instance. */
static EmulatorWindow   qemulator[1];

// Set to 1 to use an EmuGL sub-window to display GpU content, or 0 to use
// the frame post callback to retrieve every frame from the GPU, which will
// be slower, except for software-based renderers.
static bool s_use_emugl_subwindow = 1;

static void emulator_window_refresh(EmulatorWindow* emulator);
extern void qemu_system_shutdown_request(void);

static void write_window_name(char* buff,
                              size_t buff_len,
                              int base_port,
                              const char* avd_name) {
    snprintf(buff, buff_len, "%d:%s", base_port, avd_name);
}


static void
emulator_window_light_brightness(void* opaque, const char*  light, int  value)
{
    EmulatorWindow*  emulator = opaque;

    VERBOSE_PRINT(hw_control,"%s: light='%s' value=%d ui=%p", __FUNCTION__, light, value, emulator->ui);

    if (!strcmp(light, "lcd_backlight")) {
        skin_ui_set_lcd_brightness(emulator->ui, value);
        return;
    }
}

static void emulator_window_trackball_event(int dx, int dy) {
    user_event_mouse(dx, dy, 1, 0);
}

static void emulator_window_window_key_event(unsigned keycode, int down) {
    user_event_key(keycode, down);
}

static void emulator_window_window_mouse_event(unsigned x,
                                         unsigned y,
                                         unsigned state) {
    /* NOTE: the 0 is used in hw/android/goldfish/events_device.c to
     * differentiate between a touch-screen and a trackball event
     */
    user_event_mouse(x, y, 0, state);
}

static void emulator_window_window_generic_event(int event_type,
                                           int event_code,
                                           int event_value) {
    user_event_generic(event_type, event_code, event_value);
    /* XXX: hack, replace by better code here */
    if (event_value != 0)
        android_sensors_set_coarse_orientation(ANDROID_COARSE_PORTRAIT);
    else
        android_sensors_set_coarse_orientation(ANDROID_COARSE_LANDSCAPE);
}

static bool emulator_window_network_toggle(void) {
    qemu_net_disable = !qemu_net_disable;
    if (android_modem) {
        amodem_set_data_registration(
                android_modem,
        qemu_net_disable ? A_REGISTRATION_UNREGISTERED
            : A_REGISTRATION_HOME);
    }
    return !qemu_net_disable;
}

static void emulator_window_framebuffer_invalidate(void) {
    qframebuffer_invalidate_all();
    qframebuffer_check_updates();
}

static void emulator_window_keyboard_event(void* opaque, SkinKeyCode keycode, int down) {
    (void)opaque;
    user_event_key(keycode, down);
}

static int emulator_window_opengles_show_window(
    void* window, int x, int y, int w, int h, float rotation) {
    if (s_use_emugl_subwindow) {
        return android_showOpenglesWindow(window, x, y, w, h, rotation);
    } else {
        return 0;
    }
}

static int emulator_window_opengles_hide_window(void) {
    if (s_use_emugl_subwindow) {
        return android_hideOpenglesWindow();
    } else {
        return 0;
    }
}

static void emulator_window_opengles_redraw_window(void) {
    if (s_use_emugl_subwindow) {
        android_redrawOpenglesWindow();
    }
}

// Used as an emugl callback to get each frame of GPU display.
static void _emulator_window_on_gpu_frame(void* context,
                                          int width,
                                          int height,
                                          const void* pixels) {
    EmulatorWindow* emulator = (EmulatorWindow*)context;
    // This function is called from an EmuGL thread, which cannot
    // call the skin_ui_update_gpu_frame() function. Create a GpuFrame
    // instance, and send its address into the pipe.
    skin_ui_update_gpu_frame(emulator->ui, width, height, pixels);
}

static void
emulator_window_setup( EmulatorWindow*  emulator )
{
    static const SkinWindowFuncs my_window_funcs = {
        .key_event = &emulator_window_window_key_event,
        .mouse_event = &emulator_window_window_mouse_event,
        .generic_event = &emulator_window_window_generic_event,
        .opengles_show = &emulator_window_opengles_show_window,
        .opengles_hide = &emulator_window_opengles_hide_window,
        .opengles_redraw = &emulator_window_opengles_redraw_window,
        .opengles_free = &android_stopOpenglesRenderer,
    };

    static const SkinTrackBallParameters my_trackball_params = {
        .diameter = 30,
        .ring = 2,
        .ball_color = 0xffe0e0e0,
        .dot_color = 0xff202020,
        .ring_color = 0xff000000,
        .event_func = &emulator_window_trackball_event,
    };

    if (emulator->opts->no_window || emulator->ui) {
        return;
    }

    SkinUIParams my_ui_params = {
        .enable_touch = !androidHwConfig_isScreenNoTouch(android_hw),
        .enable_dpad = android_hw->hw_dPad != 0,
        .enable_keyboard = android_hw->hw_keyboard != 0,
        .enable_trackball = android_hw->hw_trackBall != 0,

        .window_x = emulator->win_x,
        .window_y = emulator->win_y,
        .window_scale = get_default_scale(emulator->opts),

        .keyboard_charmap = emulator->opts->charmap,
        .keyboard_raw_keys = emulator->opts->raw_keys != 0,
    };

    write_window_name(my_ui_params.window_name,
                      sizeof(my_ui_params.window_name),
                      android_base_port,
                      avdInfo_getName(android_avdInfo));

    static const SkinUIFuncs my_ui_funcs = {
        .window_funcs = &my_window_funcs,
        .trackball_params = &my_trackball_params,
        .keyboard_event = &emulator_window_keyboard_event, //user_event_key,
        .keyboard_flush = &user_event_keycodes,
        .network_toggle = &emulator_window_network_toggle,
        .framebuffer_invalidate = &emulator_window_framebuffer_invalidate,
    };

    emulator->ui = skin_ui_create(emulator->layout_file,
                                  android_hw->hw_initialOrientation,
                                  &my_ui_funcs,
                                  &my_ui_params);
    if (!emulator->ui) {
        return;
    }

    if (emulator->onion) {
        skin_ui_set_onion(emulator->ui,
                          emulator->onion,
                          emulator->onion_rotation,
                          emulator->onion_alpha);
    }

    // Determine whether to use an EmuGL sub-window or not.
    const char* env = getenv("ANDROID_EMULATOR_EXPERIMENT_READ_PIXELS");
    s_use_emugl_subwindow = !env || !env[0] || env[0] == '0';

    if (s_use_emugl_subwindow) {
        VERBOSE_PRINT(gles, "Using EmuGL sub-window for GPU display");
    } else {
        VERBOSE_PRINT(gles, "Using glReadPixels() for GPU display");
        gpu_frame_set_post_callback(looper_newCore(),
                                    emulator,
                                    _emulator_window_on_gpu_frame);
    }

    skin_ui_reset_title(emulator->ui);
}

static void
emulator_window_fb_update( void*   _emulator, int  x, int  y, int  w, int  h )
{
    EmulatorWindow*  emulator = _emulator;

    if (emulator->opts->no_window) {
        return;
    }

    if (!emulator->ui) {
        emulator_window_setup(emulator);
    }
    skin_ui_update_display(emulator->ui, x, y, w, h);
}

static void
emulator_window_fb_rotate( void*  _emulator, int  rotation )
{
    EmulatorWindow*  emulator = _emulator;

    emulator_window_setup( emulator );
}

static void
emulator_window_fb_poll( void* _emulator )
{
    EmulatorWindow* emulator = _emulator;
    emulator_window_refresh(emulator);
}

EmulatorWindow*
emulator_window_get(void)
{
    return qemulator;
}

static void emulator_window_framebuffer_free(void* opaque) {
    QFrameBuffer* fb = opaque;

    qframebuffer_done(fb);
    free(fb);
}

static void* emulator_window_framebuffer_create(int width, int height, int bpp) {
    QFrameBuffer* fb = calloc(1, sizeof(*fb));

    qframebuffer_init(fb, width, height, 0,
                      bpp == 32 ? QFRAME_BUFFER_RGBX_8888
                                : QFRAME_BUFFER_RGB565 );

    qframebuffer_fifo_add(fb);
    return fb;
}

static void* emulator_window_framebuffer_get_pixels(void* opaque) {
    QFrameBuffer* fb = opaque;
    return fb->pixels;
}

static int emulator_window_framebuffer_get_depth(void* opaque) {
    QFrameBuffer* fb = opaque;
    return fb->bits_per_pixel;
}

int
emulator_window_init( EmulatorWindow*       emulator,
                AConfig*         aconfig,
                const char*      basepath,
                int              x,
                int              y,
                AndroidOptions*  opts )
{
    static const SkinFramebufferFuncs skin_fb_funcs = {
        .create_framebuffer = &emulator_window_framebuffer_create,
        .free_framebuffer = &emulator_window_framebuffer_free,
        .get_pixels = &emulator_window_framebuffer_get_pixels,
        .get_depth = &emulator_window_framebuffer_get_depth,
    };

    emulator->aconfig = aconfig;
    emulator->layout_file =
            skin_file_create_from_aconfig(aconfig,
                                          basepath,
                                          &skin_fb_funcs);
    emulator->ui = NULL;
    emulator->win_x = x;
    emulator->win_y = y;
    emulator->opts[0] = opts[0];

    /* register as a framebuffer clients for all displays defined in the skin file */
    SKIN_FILE_LOOP_PARTS( emulator->layout_file, part )
        SkinDisplay*  disp = part->display;
        if (disp->valid) {
            qframebuffer_add_client( disp->framebuffer,
                                    emulator,
                                    emulator_window_fb_update,
                                    emulator_window_fb_rotate,
                                    emulator_window_fb_poll,
                                    NULL );
        }
    SKIN_FILE_LOOP_END_PARTS

    /* initialize hardware control support */
    AndroidHwControlFuncs funcs;
    funcs.light_brightness = emulator_window_light_brightness;
    android_hw_control_set(emulator, &funcs);

    return 0;
}

void
emulator_window_done(EmulatorWindow* emulator)
{
    if (emulator->ui) {
        skin_ui_free(emulator->ui);
        emulator->ui = NULL;
    }
    if (emulator->layout_file) {
        skin_file_free(emulator->layout_file);
        emulator->layout_file = NULL;
    }
}

QFrameBuffer*
emulator_window_get_first_framebuffer(EmulatorWindow* emulator)
{
    /* register as a framebuffer clients for all displays defined in the skin file */
    SKIN_FILE_LOOP_PARTS( emulator->layout_file, part )
        SkinDisplay*  disp = part->display;
        if (disp->valid) {
            return disp->framebuffer;
        }
    SKIN_FILE_LOOP_END_PARTS
    return NULL;
}

/*
 * Helper routines
 */

static int
get_device_dpi( AndroidOptions*  opts )
{
    int dpi_device = android_hw->hw_lcd_density;

    if (opts->dpi_device != NULL) {
        char*  end;
        dpi_device = strtol( opts->dpi_device, &end, 0 );
        if (end == NULL || *end != 0 || dpi_device <= 0) {
            fprintf(stderr, "argument for -dpi-device must be a positive integer. Aborting\n" );
            exit(1);
        }
    }
    return  dpi_device;
}

static double
get_default_scale( AndroidOptions*  opts )
{
    int     dpi_device  = get_device_dpi( opts );
    int     dpi_monitor = -1;
    double  scale       = 0.0;

    /* possible values for the 'scale' option are
     *   'auto'        : try to determine the scale automatically
     *   '<number>dpi' : indicates the host monitor dpi, compute scale accordingly
     *   '<fraction>'  : use direct scale coefficient
     */

    if (opts->scale) {
        if (!strcmp(opts->scale, "auto"))
        {
            /* we need to get the host dpi resolution ? */
            int   xdpi, ydpi;

            if (skin_winsys_get_monitor_dpi(&xdpi, &ydpi) < 0) {
                fprintf(stderr, "could not get monitor DPI resolution from system. please use -dpi-monitor to specify one\n" );
                exit(1);
            }
            D( "system reported monitor resolutions: xdpi=%d ydpi=%d\n", xdpi, ydpi);
            dpi_monitor = (xdpi + ydpi+1)/2;
        }
        else
        {
            char*   end;
            scale = strtod( opts->scale, &end );

            if (end && end[0] == 'd' && end[1] == 'p' && end[2] == 'i' && end[3] == 0) {
                if ( scale < 20 || scale > 1000 ) {
                    fprintf(stderr, "emulator: ignoring bad -scale argument '%s': %s\n", opts->scale,
                            "host dpi number must be between 20 and 1000" );
                    exit(1);
                }
                dpi_monitor = scale;
                scale       = 0.0;
            }
            else if (end == NULL || *end != 0) {
                fprintf(stderr, "emulator: ignoring bad -scale argument '%s': %s\n", opts->scale,
                        "not a number or the 'auto' keyword" );
                exit(1);
            }
            else if ( scale < 0.1 || scale > 3. ) {
                fprintf(stderr, "emulator: ignoring bad -window-scale argument '%s': %s\n", opts->scale,
                        "must be between 0.1 and 3.0" );
                exit(1);
            }
        }
    }

    if (scale == 0.0 && dpi_monitor > 0)
        scale = dpi_monitor*1.0/dpi_device;

    return scale;
}


/* called periodically to poll for user input events */
static void emulator_window_refresh(EmulatorWindow* emulator)
{
   /* this will eventually call sdl_update if the content of the VGA framebuffer
    * has changed */
    qframebuffer_check_updates();

    if (emulator->ui) {
        if (skin_ui_process_events(emulator->ui)) {
            // Quit program.
            skin_ui_free(emulator->ui);
            emulator->ui = NULL;
            qemu_system_shutdown_request();
        }
    }
}

/*
 * android/console.c helper routines.
 */

void
android_emulator_set_window_scale(double  scale, int  is_dpi)
{
    EmulatorWindow*  emulator = qemulator;

    if (is_dpi)
        scale /= get_device_dpi( emulator->opts );

    if (emulator->ui) {
        skin_ui_set_scale(emulator->ui, scale);
    }
}


void
android_emulator_set_base_port( int  port )
{
    if (qemulator->ui) {
        /* Base port is already set in the emulator's core. */
        char buff[32];
        write_window_name(buff,
                        sizeof(buff),
                        android_base_port,
                        avdInfo_getName(android_avdInfo));

        skin_ui_set_name(qemulator->ui, buff);
    }
}

SkinLayout*
emulator_window_get_layout(EmulatorWindow* emulator)
{
    if (emulator->ui) {
        return skin_ui_get_current_layout(emulator->ui);
    } else {
        return emulator->layout_file->layouts;
    }
}
