/* Copyright (C) 2006-2016 The Android Open Source Project
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

#include "android/skin/ui.h"

#include <stdbool.h>                     // for bool, true, false
#include <stdio.h>                       // for NULL, snprintf
#include <string.h>                      // for strcmp

#include "android/emulator-window.h"     // for emulator_window_set_no_skin
#include "android/skin/event.h"          // for SkinEvent, (anonymous struct...
#include "android/skin/file.h"           // for SkinLayout, SkinFile, skin_l...
#include "android/skin/generic-event.h"  // for skin_generic_event_create
#include "android/skin/image.h"          // for skin_image_unref, skin_image...
#include "android/skin/keyboard.h"       // for skin_keyboard_process_event
#include "android/skin/rect.h"           // for SkinRotation
#include "android/skin/trackball.h"      // for skin_trackball_create, skin_...
#include "android/skin/window.h"         // for skin_window_process_event
#include "android/utils/bufprint.h"      // for bufprint
#include "android/utils/debug.h"         // for dprint, VERBOSE_CHECK, VERBO...
#include "android/utils/system.h"        // for AFREE, ANEW0

#ifdef _WIN32
#include <windows.h>
#endif

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)
#define  DE(...) do { if (VERBOSE_CHECK(keys)) dprint(__VA_ARGS__); } while (0)

struct SkinUI {
    SkinUIParams           ui_params;
    const SkinUIFuncs*     ui_funcs;

    SkinFile*              layout_file;
    SkinLayout*            layout;

    SkinKeyboard*          keyboard;
    SkinGenericEvent*      generic_events;

    SkinWindow*            window;

    bool                   show_trackball;
    SkinTrackBall*         trackball;

    int                    lcd_brightness;

    SkinImage*             onion;
    SkinRotation           onion_rotation;
    int                    onion_alpha;
};

static SkinLayout* skin_file_select_layout(SkinLayout* layouts,
        const char* layout_name) {
    if (!layout_name) return layouts;
    SkinLayout* currLayout = layouts;
    while (currLayout) {
        if (currLayout->name && !strcmp(currLayout->name, layout_name)) {
            return currLayout;
        }
        currLayout = currLayout->next;
    }
    return layouts;
}

SkinUI* skin_ui_create(SkinFile* layout_file,
                       const char* initial_orientation,
                       const SkinUIFuncs* ui_funcs,
                       const SkinUIParams* ui_params,
                       bool use_emugl_subwindow) {

    SkinUI* ui;

    ANEW0(ui);

    ui->layout_file = layout_file;
    ui->layout = skin_file_select_layout(layout_file->layouts, initial_orientation);

    ui->ui_funcs = ui_funcs;
    ui->ui_params = ui_params[0];

    ui->keyboard = skin_keyboard_create(ui->ui_params.keyboard_charmap,
                                        ui->layout->dpad_rotation,
                                        ui_funcs->keyboard_flush);
    ui->generic_events =
            skin_generic_event_create(ui_funcs->generic_event_flush);
    ui->window = NULL;

    ui->window = skin_window_create(
            ui->layout, ui->ui_params.window_x, ui->ui_params.window_y,
            ui->ui_params.enable_scale,
            use_emugl_subwindow, ui->ui_funcs->window_funcs);
    if (!ui->window) {
        skin_ui_free(ui);
        return NULL;
    }

    if (ui->ui_params.enable_trackball) {
        ui->trackball = skin_trackball_create(ui->ui_funcs->trackball_params);
        skin_window_set_trackball(ui->window, ui->trackball);
    }

    ui->lcd_brightness = 128;  /* 50% */
    skin_window_set_lcd_brightness(ui->window, ui->lcd_brightness );

    if (ui->onion) {
        skin_window_set_onion(ui->window,
                              ui->onion,
                              ui->onion_rotation,
                              ui->onion_alpha);
    }

    skin_ui_reset_title(ui);

    skin_window_enable_touch(ui->window, ui->ui_params.enable_touch);
    skin_window_enable_dpad(ui->window, ui->ui_params.enable_dpad);
    skin_window_enable_qwerty(ui->window, ui->ui_params.enable_keyboard);
    skin_window_enable_trackball(ui->window, ui->ui_params.enable_trackball);

    return ui;
}

void skin_ui_free(SkinUI* ui) {
    if (ui->window) {
        skin_window_free(ui->window);
        ui->window = NULL;
    }
    if (ui->trackball) {
        skin_trackball_destroy(ui->trackball);
        ui->trackball = NULL;
    }
    if (ui->keyboard) {
        skin_keyboard_free(ui->keyboard);
        ui->keyboard = NULL;
    }
    if (ui->generic_events) {
        skin_generic_event_free(ui->generic_events);
        ui->generic_events = NULL;
    }

    skin_image_unref(&ui->onion);

    ui->layout = NULL;

    AFREE(ui);
}

void skin_ui_set_lcd_brightness(SkinUI* ui, int lcd_brightness) {
    ui->lcd_brightness = lcd_brightness;
    if (ui->window) {
        skin_window_set_lcd_brightness(ui->window, lcd_brightness);
    }
}

void skin_ui_set_scale(SkinUI* ui, double scale) {
    if (ui->window) {
        skin_window_set_scale(ui->window, scale);
    }
}

void skin_ui_reset_title(SkinUI* ui) {
    char  temp[128], *p=temp, *end = p + sizeof(temp);

    if (ui->window == NULL)
        return;

    if (ui->show_trackball) {
        p = bufprint(p, end, "Press Ctrl-T to leave trackball mode. ");
    }

    p = bufprint(p, end, "%s", ui->ui_params.window_name);
    skin_window_set_title(ui->window, temp);
}

void skin_ui_set_onion(SkinUI* ui,
                       SkinImage* onion,
                       SkinRotation onion_rotation,
                       int onion_alpha) {
    if (onion) {
        skin_image_ref(onion);
    }
    skin_image_unref(&ui->onion);

    ui->onion = onion;
    ui->onion_rotation = onion_rotation;
    ui->onion_alpha = onion_alpha;

    if (ui->window) {
        skin_window_set_onion(ui->window,
                              onion,
                              onion_rotation,
                              onion_alpha);
    }
}

static void skin_ui_switch_to_layout(SkinUI* ui, SkinLayout* layout) {
    ui->layout = layout;
    skin_window_reset(ui->window, layout);
    SkinRotation rotation = skin_layout_get_dpad_rotation(layout);

    if (ui->keyboard) {
        skin_keyboard_set_rotation(ui->keyboard, rotation);
    }

    if (ui->trackball) {
        skin_trackball_set_rotation(ui->trackball, rotation);
        skin_window_set_trackball(ui->window, ui->trackball);
        skin_window_show_trackball(ui->window, ui->show_trackball);
    }

    skin_window_set_lcd_brightness(ui->window, ui->lcd_brightness);
    ui->ui_funcs->framebuffer_invalidate();
}

static void _skin_ui_handle_rotate_key_command(SkinUI* ui, bool next) {
    SkinLayout* layout = NULL;

    if (next) {
        layout = ui->layout->next;
        if (layout == NULL)
            layout = ui->layout_file->layouts;
    } else {
        layout = ui->layout_file->layouts;
        while (layout->next && layout->next != ui->layout)
            layout = layout->next;
    }
    if (layout != NULL) {
        skin_ui_switch_to_layout(ui, layout);
    }
}

void skin_ui_select_next_layout(SkinUI* ui) {
    _skin_ui_handle_rotate_key_command(ui, true);
}

void skin_ui_update_rotation(SkinUI* ui, SkinRotation rotation) {
    skin_window_update_rotation(ui->window, rotation);
}

bool skin_ui_rotate(SkinUI* ui, SkinRotation rotation) {
    SkinLayout* l;
    for (l = ui->layout_file->layouts;
         l;
         l = l->next) {
        if (l->orientation == rotation) {
            skin_ui_switch_to_layout(ui, l);
            return true;
        }
    }
    return false;
}

bool skin_ui_update_and_rotate(SkinUI* ui,
                               SkinFile* layout_file,
                               SkinRotation rotation) {
    ui->layout_file = layout_file;
    skin_ui_rotate(ui, rotation);
    return true;
}

bool skin_ui_process_events(SkinUI* ui) {
    SkinEvent ev;

    // If a scrolled window is zoomed or resized while the scroll bars
    // are moved, Qt window scroll events are created as the window resizes.
    // They will be in the event queue behind the set-scale or set-zoom. Because
    // scroll events work by "moving" the GL sub-window when using host GPU and
    // finding its intersection with the Qt window, scroll events produced by a
    // resize should be ignored, since they may move the GL sub-window far enough
    // that it no longer intersects the Qt window at its current size.
    bool ignoreScroll = false;

    // Enable mouse tracking so we can change the cursor when it's at a corner
    skin_enable_mouse_tracking(true);

    while(skin_event_poll(&ev)) {
        switch(ev.type) {
        case kEventForceRedraw:
            DE("EVENT: kEventForceRedraw");
            skin_window_redraw(ui->window, NULL);
            break;

        case kEventKeyDown:
            DE("EVENT: kEventKeyDown scancode=%d mod=0x%x",
               ev.u.key.keycode, ev.u.key.mod);
            skin_keyboard_process_event(ui->keyboard, &ev, 1);
            break;

        case kEventKeyUp:
            DE("EVENT: kEventKeyUp scancode=%d mod=0x%x",
               ev.u.key.keycode, ev.u.key.mod);
            skin_keyboard_process_event(ui->keyboard, &ev, 0);
            break;

        case kEventGeneric:
            DE("EVENT: kEventGeneric type=0x%02x code=0x%03x val=%x",
               ev.u.generic_event.type, ev.u.generic_event.code,
               ev.u.generic_event.value);
            skin_generic_event_process_event(ui->generic_events, &ev);
            break;

        case kEventTextInput:
            DE("EVENT: kEventTextInput text=[%s] down=%s",
               ev.u.text.text, ev.u.text.down ? "true" : "false");
            skin_keyboard_process_event(ui->keyboard, &ev, ev.u.text.down);
            break;

        case kEventMouseMotion:
            DE("EVENT: kEventMouseMotion x=%d y=%d xrel=%d yrel=%d button=%d",
               ev.u.mouse.x, ev.u.mouse.y, ev.u.mouse.xrel, ev.u.mouse.yrel,
               ev.u.mouse.button);
            skin_window_process_event(ui->window, &ev);
            break;
        case kEventLayoutRotate:
            DE("EVENT: kEventLayoutRotate orientation=%d",
                ev.u.layout_rotation.rotation);
            skin_ui_rotate(ui, ev.u.layout_rotation.rotation);
            break;
        case kEventMouseButtonDown:
        case kEventMouseButtonUp:
            DE("EVENT: kEventMouseButton x=%d y=%d xrel=%d yrel=%d button=%d",
               ev.u.mouse.x, ev.u.mouse.y, ev.u.mouse.xrel, ev.u.mouse.yrel,
               ev.u.mouse.button);
            skin_window_process_event(ui->window, &ev);
            break;
        case kEventTouchBegin:
        case kEventTouchEnd:
        case kEventTouchUpdate:

            DE("EVENT: kEventMultiTouch x=%f y=%f id=%d", ev.u.multi_touch_point.x,
               ev.u.multi_touch_point.y, ev.u.multi_touch_point.id);
            skin_window_process_touch_event(ui->window, &ev);
            break;

        case kEventMouseWheel:
            DE("EVENT: kEventMouseWheel x_delta=%d y_delta=%d",
               ev.u.wheel.x_delta, ev.u.wheel.y_delta);
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventMouseStartTracking:
            DE("EVENT: kEventMouseStartTracking");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventMouseStopTracking:
            DE("EVENT: kEventMouseStopTracking");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventPenMove:
            DE("EVENT: kEventPenMove x=%d y=%d id=%d pressure=%d \
                orientation=%d button=%d pointer=%d",
                ev.u.pen.x, ev.u.pen.y, ev.u.pen.tracking_id,
                ev.u.pen.pressure, ev.u.pen.orientation,
                ev.u.pen.button_pressed, ev.u.pen.rubber_pointer);
            skin_window_process_pen_event(ui->window, &ev);
            break;
        case kEventPenPress:
        case kEventPenRelease:
            DE("EVENT: kEventPen x=%d y=%d id=%d pressure=%d \
                orientation=%d button=%d pointer=%d",
                ev.u.pen.x, ev.u.pen.y, ev.u.pen.tracking_id,
                ev.u.pen.pressure, ev.u.pen.orientation,
                ev.u.pen.button_pressed, ev.u.pen.rubber_pointer);
            skin_window_process_pen_event(ui->window, &ev);
            break;

        case kEventScrollBarChanged:
            DE("EVENT: kEventScrollBarChanged x=%d xmax=%d y=%d ymax=%d ignored=%d",
               ev.u.scroll.x, ev.u.scroll.xmax, ev.u.scroll.y, ev.u.scroll.ymax, ignoreScroll);
            if (!ignoreScroll) {
                skin_window_scroll_updated(ui->window, ev.u.scroll.x, ev.u.scroll.xmax,
                                                       ev.u.scroll.y, ev.u.scroll.ymax);
            }
            break;
        case kEventRotaryInput:
            DE("EVENT: kEventRotaryInput delta=%d",
               ev.u.rotary_input.delta);
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventSetDisplayRegion:
            DE("EVENT: kEventSetDisplayRegion (%d, %d) %d x %d",
               ev.u.display_region.xOffset, ev.u.display_region.yOffset,
               ev.u.display_region.width, ev.u.display_region.height);

            skin_window_set_layout_region(ui->window,
                                          ev.u.display_region.xOffset, ev.u.display_region.yOffset,
                                          ev.u.display_region.width,   ev.u.display_region.height);
            break;

        case kEventSetDisplayRegionAndUpdate:
            DE("EVENT: kEventSetDisplayRegionAndUpdate (%d, %d) %d x %d",
               ev.u.display_region.xOffset, ev.u.display_region.yOffset,
               ev.u.display_region.width, ev.u.display_region.height);
            skin_window_set_display_region_and_update(ui->window,
                                                      ev.u.display_region.xOffset,
                                                      ev.u.display_region.yOffset,
                                                      ev.u.display_region.width,
                                                      ev.u.display_region.height);
            break;

        case kEventSetScale:
            DE("EVENT: kEventSetScale scale=%f", ev.u.window.scale);
            ignoreScroll = true;
            skin_window_set_scale(ui->window, ev.u.window.scale);
            break;

        case kEventSetZoom:
            DE("EVENT: kEventSetZoom x=%d y=%d zoom=%f scroll_h=%d",
               ev.u.window.x, ev.u.window.y, ev.u.window.scale, ev.u.window.scroll_h);
            skin_window_set_zoom(ui->window, ev.u.window.scale, ev.u.window.x, ev.u.window.y,
                                             ev.u.window.scroll_h);
            break;

        case kEventQuit:
            DE("EVENT: kEventQuit");
            /* only save emulator config through clean exit */
            return true;

        case kEventWindowMoved:
            DE("EVENT: kEventWindowMoved x=%d y=%d", ev.u.window.x, ev.u.window.y);
            skin_window_position_changed(ui->window, ev.u.window.x, ev.u.window.y);
            break;

        case kEventScreenChanged:
            DE("EVENT: kEventScreenChanged");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventWindowChanged:
            DE("EVENT: kEventWindowChanged");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventZoomedWindowResized:
            DE("EVENT: kEventZoomedWindowResized dx=%d dy=%d w=%d h=%d",
               ev.u.scroll.x, ev.u.scroll.y, ev.u.scroll.xmax, ev.u.scroll.ymax);
            skin_window_zoomed_window_resized(ui->window, ev.u.scroll.x, ev.u.scroll.y,
                                                          ev.u.scroll.xmax, ev.u.scroll.ymax,
                                                          ev.u.scroll.scroll_h);
            break;
        case kEventToggleTrackball:
            if (ui->ui_params.enable_trackball) {
                ui->show_trackball = !ui->show_trackball;
                skin_window_show_trackball(ui->window, ui->show_trackball);
                skin_ui_reset_title(ui);
            }
            break;
        case kEventSetNoSkin:
            emulator_window_set_no_skin();
            break;
        case kEventRestoreSkin:
            emulator_window_restore_skin();
            break;
        case kEventSetDisplayActiveConfig:
            emulator_window_opengles_set_display_active_config(ev.u.display_active_config);
            break;
        case kEventAddDisplay:
            DE("EVENT: kEventAddDisplay");
            skin_window_add_display(ui->window, ev.u.add_display.id, ev.u.add_display.width, ev.u.add_display.height);
            break;
        case kEventRemoveDisplay:
            DE("EVENT: kEventRemoveDisplay");
            skin_window_remove_display(ui->window, ev.u.remove_display.id);
            break;
        default: ;
        }
    }

    skin_keyboard_flush(ui->keyboard);
    return false;
}

void skin_ui_update_display(SkinUI* ui, int x, int y, int w, int h) {
    if (ui->window) {
        skin_window_update_display(ui->window, x, y, w, h);
    }
}

void skin_ui_update_gpu_frame(SkinUI* ui, int w, int h, const void* pixels) {
    if (ui->window) {
        skin_window_update_gpu_frame(ui->window, w, h, pixels);
    }
}

SkinLayout* skin_ui_get_current_layout(const SkinUI* ui) {
    return ui->layout;
}

SkinLayout* skin_ui_get_next_layout(const SkinUI* ui) {
    return ui->layout->next ? ui->layout->next : ui->layout_file->layouts;
}

SkinLayout* skin_ui_get_prev_layout(const SkinUI* ui) {
    SkinLayout* layout = ui->layout_file->layouts;
    while (layout->next && layout->next != ui->layout) {
        layout = layout->next;
    }
    return layout;
}

void skin_ui_set_name(SkinUI* ui, const char* name) {
    snprintf(ui->ui_params.window_name,
             sizeof(ui->ui_params.window_name),
             "%s",
             name);
    skin_ui_reset_title(ui);
}

bool skin_ui_is_trackball_active(SkinUI* ui) {
    return (ui && ui->ui_params.enable_trackball && ui->show_trackball);
}
