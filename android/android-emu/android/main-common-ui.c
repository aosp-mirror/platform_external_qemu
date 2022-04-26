/* Copyright (C) 2015 The Android Open Source Project
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

#include "android/main-common-ui.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <assert.h>   // for assert
#include <ctype.h>    // for isdigit
#include <signal.h>   // for signal, SIG_DFL
#include <stdio.h>    // for NULL, snprintf
#include <stdlib.h>   // for atoi, exit, free
#include <string.h>   // for strdup, strchr

#include "android/avd/util.h"
#include "android/boot-properties.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/feature_control.h"
#include "android/framebuffer.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/resource.h"
#include "android/skin/charmap.h"
#include "android/skin/file.h"
#include "android/skin/image.h"
#include "android/skin/keyboard.h"
#include "android/skin/resource.h"
#include "android/skin/trackball.h"
#include "android/skin/window.h"
#include "android/skin/winsys.h"
#include "android/user-config.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"
#include "android/utils/string.h"

#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(init)) \
            dprint(__VA_ARGS__); \
    } while (0)

/***  CONFIGURATION
 ***/

static AConfig* s_skinConfig = NULL;
static char* s_skinPath = NULL;
static int s_deviceLcdWidth = 0;
static int s_deviceLcdHeight = 0;

bool emulator_initMinimalSkinConfig(
    int lcd_width, int lcd_height,
    const SkinRect* rect,
    AUserConfig** userConfig_out);

bool user_config_init(void) {
    AUserConfig* userConfig = NULL;
    SkinRect mmmRect;
    skin_winsys_get_monitor_rect(&mmmRect);
    if (min_config_qemu_mode) {
        emulator_initMinimalSkinConfig(android_hw->hw_lcd_width,
                                       android_hw->hw_lcd_height, &mmmRect,
                                       &userConfig);
    } else {
        userConfig = auserConfig_new(getConsoleAgents()->settings->avdInfo(), &mmmRect, s_deviceLcdWidth, s_deviceLcdHeight);
    }
    getConsoleAgents()->settings->inject_userConfig(userConfig);
    return userConfig != NULL;
}

/* only call this function on normal exits, so that ^C doesn't save the
 * configuration */
void user_config_done(void) {
    int win_x, win_y;

    AUserConfig* userConfig = getConsoleAgents()->settings->userConfig();
    if (!userConfig) {
        D("no user configuration?");
        return;
    }

    skin_winsys_get_window_pos(&win_x, &win_y);
    auserConfig_setWindowPos(userConfig, win_x, win_y);
    auserConfig_save(userConfig);
    auserConfig_free(userConfig);
    getConsoleAgents()->settings->inject_userConfig(NULL);
}

void user_config_get_window_pos(int* window_x, int* window_y) {
    *window_x = *window_y = 10;

    AUserConfig* userConfig = getConsoleAgents()->settings->userConfig();
    if (userConfig)
        auserConfig_getWindowPos(userConfig, window_x, window_y);
}

/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****            S K I N   S U P P O R T                          *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

/* list of skin aliases */
static const struct {
    const char* name;
    const char* alias;
} skin_aliases[] = {{"QVGA-L", "320x240"}, {"QVGA-P", "240x320"},
                    {"HVGA-L", "480x320"}, {"HVGA-P", "320x480"},
                    {"QVGA", "320x240"},   {"HVGA", "320x480"},
                    {NULL, NULL}};

void parse_skin_files(const char* skinDirPath,
                      const char* skinName,
                      AndroidOptions* opts,
                      AndroidHwConfig* hwConfig,
                      AConfig** skinConfig,
                      char** skinPath) {
    char tmp[1024];
    AConfig* root;
    const char* path = NULL;
    AConfig* n;

    root = aconfig_node("", "");

    if (skinName == NULL)
        goto DEFAULT_SKIN;

    /* Support skin aliases like QVGA-H QVGA-P, etc...
       But first we check if it's a directory that exist before applying
       the alias */
    int checkAlias = 1;

    if (skinDirPath != NULL) {
        bufprint(tmp, tmp + sizeof(tmp), "%s" PATH_SEP "%s", skinDirPath,
                 skinName);
        if (path_exists(tmp)) {
            checkAlias = 0;
        } else {
            D("there is no '%s' skin in '%s'", skinName, skinDirPath);
        }
    }

    if (checkAlias) {
        int nn;

        for (nn = 0;; nn++) {
            const char* skin_name = skin_aliases[nn].name;
            const char* skin_alias = skin_aliases[nn].alias;

            if (!skin_name)
                break;

            if (!strcasecmp(skin_name, skinName)) {
                D("skin name '%s' aliased to '%s'", skinName, skin_alias);
                skinName = skin_alias;
                break;
            }
        }
    }

    /* Magically support skins like "320x240" or "320x240x16" */
    if (isdigit(skinName[0])) {
        char* x = strchr(skinName, 'x');
        if (x && isdigit(x[1])) {
            int width = atoi(skinName);
            int height = atoi(x + 1);
            int bpp = hwConfig->hw_lcd_depth;  // respect the depth setting in
                                               // the config.ini
            char* y = strchr(x + 1, 'x');
            if (y && isdigit(y[1])) {
                bpp = atoi(y + 1);
            }

            snprintf(tmp, sizeof tmp,
                     "display {\n  width %d\n  height %d\n bpp %d}\n", width,
                     height, bpp);
            aconfig_load(root, strdup(tmp));
            path = ":";
            D("found magic skin width=%d height=%d bpp=%d\n", width, height,
              bpp);
            goto FOUND_SKIN;
        }
    }

    if (skinDirPath == NULL) {
        derror("unknown skin name '%s'", skinName);
        exit(1);
    }

    snprintf(tmp, sizeof tmp, "%s" PATH_SEP "%s" PATH_SEP "layout", skinDirPath,
             skinName);
    D("trying to load skin file '%s'", tmp);

    if (aconfig_load_file(root, tmp) < 0) {
        dwarning("could not load skin file '%s', using built-in one\n", tmp);
        goto DEFAULT_SKIN;
    }

    snprintf(tmp, sizeof tmp, "%s" PATH_SEP "%s" PATH_SEP, skinDirPath,
             skinName);
    path = tmp;
    goto FOUND_SKIN;

FOUND_SKIN:
    /* the default network speed and latency can now be specified by the device
     * skin */
    n = aconfig_find(root, "network");
    if (n != NULL) {
        android_skin_net_speed = aconfig_str(n, "speed", 0);
        android_skin_net_delay = aconfig_str(n, "delay", 0);
    }

    /* extract framebuffer information from the skin.
     *
     * for version 1 of the skin format, they are in the top-level
     * 'display' element.
     *
     * for version 2 of the skin format, they are under parts.device.display
     */
    n = aconfig_find(root, "display");
    if (n == NULL) {
        n = aconfig_find(root, "parts");
        if (n != NULL) {
            n = aconfig_find(n, "device");
            if (n != NULL) {
                n = aconfig_find(n, "display");
            }
        }
    }

    if (n != NULL) {
        int width = aconfig_int(n, "width", hwConfig->hw_lcd_width);
        int height = aconfig_int(n, "height", hwConfig->hw_lcd_height);
        int depth = aconfig_int(n, "bpp", hwConfig->hw_lcd_depth);

        if (width > 0 && height > 0) {
            /* The emulated framebuffer wants a width that is a multiple of 2 */
            if ((width & 1) != 0) {
                width = (width + 1) & ~1;
                D("adjusting LCD dimensions to (%dx%dx)", width, height);
            }

            /* only depth values of 16 and 32 are correct. 16 is the default. */
            if (depth != 32 && depth != 16) {
                depth = 16;
                D("adjusting LCD bit depth to %d", depth);
            }

            hwConfig->hw_lcd_width = width;
            hwConfig->hw_lcd_height = height;
            hwConfig->hw_lcd_depth = depth;
        } else {
            D("ignoring invalid skin LCD dimensions (%dx%dx%d)", width, height,
              depth);
        }
    }

    *skinConfig = root;
    *skinPath = strdup(path);
    return;

DEFAULT_SKIN : {
    const unsigned char* layout_base;
    size_t layout_size;
    char* base;

    skinName = "<builtin>";

    layout_base = skin_resource_find("layout", &layout_size);
    if (layout_base == NULL) {
        dfatal("Couldn't load builtin skin");
        exit(1);
    }
    base = malloc(layout_size + 1);
    memcpy(base, layout_base, layout_size);
    base[layout_size] = 0;

    D("parsing built-in skin layout file (%d bytes)", (int)layout_size);
    aconfig_load(root, base);
    path = ":";
}
    goto FOUND_SKIN;
}

void create_minimal_skin_config(int lcd_width,
                                int lcd_height,
                                AConfig** skinConfig,
                                char** skinPath) {
    char tmp[1024];
    AConfig* root;
    const char* path = NULL;
    AConfig* n;

    root = aconfig_node("", "");

    int width = lcd_width;
    int height = lcd_height;
    int bpp = 32;
    snprintf(tmp, sizeof tmp, "display {\n  width %d\n  height %d\n bpp %d}\n",
             width, height, bpp);
    aconfig_load(root, strdup(tmp));
    path = ":";
    D("found magic skin width=%d height=%d bpp=%d\n", width, height, bpp);

    /* the default network speed and latency can now be specified by the device
     * skin */
    n = aconfig_find(root, "network");
    if (n != NULL) {
        android_skin_net_speed = aconfig_str(n, "speed", 0);
        android_skin_net_delay = aconfig_str(n, "delay", 0);
    }

    /* extract framebuffer information from the skin.
     *
     * for version 1 of the skin format, they are in the top-level
     * 'display' element.
     *
     * for version 2 of the skin format, they are under parts.device.display
     */
    n = aconfig_find(root, "display");
    if (n == NULL) {
        n = aconfig_find(root, "parts");
        if (n != NULL) {
            n = aconfig_find(n, "device");
            if (n != NULL) {
                n = aconfig_find(n, "display");
            }
        }
    }

    if (n != NULL) {
        int width = aconfig_int(n, "width", lcd_width);
        int height = aconfig_int(n, "height", lcd_height);
        int depth = aconfig_int(n, "bpp", 32);

        if (width > 0 && height > 0) {
            /* The emulated framebuffer wants a width that is a multiple of 2 */
            if ((width & 1) != 0) {
                width = (width + 1) & ~1;
                D("adjusting LCD dimensions to (%dx%dx)", width, height);
            }

            /* only depth values of 16 and 32 are correct. 16 is the default. */
            if (depth != 32 && depth != 16) {
                depth = 16;
                D("adjusting LCD bit depth to %d", depth);
            }

        } else {
            D("ignoring invalid skin LCD dimensions (%dx%dx%d)", width, height,
              depth);
        }
    }

    *skinConfig = root;
    *skinPath = strdup(path);
    return;
}

bool emulator_initMinimalSkinConfig(int lcd_width,
                                    int lcd_height,
                                    const SkinRect* rect,
                                    AUserConfig** userConfig_out) {
    s_deviceLcdWidth = lcd_width;
    s_deviceLcdHeight = lcd_height;

    create_minimal_skin_config(lcd_width, lcd_height, &s_skinConfig,
                               &s_skinPath);

    *userConfig_out =
        auserConfig_new_custom(
            getConsoleAgents()->settings->avdInfo(), rect,
            s_deviceLcdWidth, s_deviceLcdHeight);
    return true;
}

bool emulator_parseInputCommandLineOptions(AndroidOptions* opts) {
#ifndef CONFIG_HEADLESS
    if (opts->use_keycode_forwarding ||
        feature_is_enabled(kFeature_KeycodeForwarding)) {
            getConsoleAgents()->settings->set_keycode_forwading(true);
    }

    // Set keyboard layout for Android only.
    if (getConsoleAgents()->settings->use_keycode_forwarding() && !opts->fuchsia) {
        const char* cur_keyboard_layout = skin_keyboard_host_layout_name();
        const char* android_keyboard_layout =
                skin_keyboard_host_to_guest_layout_name(cur_keyboard_layout);
        if (android_keyboard_layout) {
            boot_property_add_qemu_keyboard_layout(android_keyboard_layout);
        } else {
// Always use keycode forwarding on Linux due to bug in prebuilt Libxkb.
#if defined(_WIN32) || defined(__APPLE__)
            D("No matching Android keyboard layout for %s. Fall back to host "
              "translation.",
              cur_keyboard_layout);
            getConsoleAgents()->settings->set_keycode_forwading(false);
#endif
        }
    }
#endif
    return true;
}

bool emulator_parseUiCommandLineOptions(AndroidOptions* opts,
                                        AvdInfo* avd,
                                        AndroidHwConfig* hw) {
    s_deviceLcdWidth = hw->hw_lcd_width;
    s_deviceLcdHeight = hw->hw_lcd_height;
    if (skin_charmap_setup(opts->charmap)) {
        return false;
    }

    parse_skin_files(opts->skindir, opts->skin, opts, hw, &s_skinConfig,
                     &s_skinPath);

    if (!opts->charmap) {
        /* Try to find a valid charmap name */
        char* charmap = avdInfo_getCharmapFile(avd, hw->hw_keyboard_charmap);
        if (charmap != NULL) {
            D("autoconfig: -charmap %s", charmap);
            str_reset_nocopy(&opts->charmap, charmap);
        }
    }

    if (opts->charmap) {
        char charmap_name[SKIN_CHARMAP_NAME_SIZE];

        if (!path_exists(opts->charmap)) {
            derror("Charmap file does not exist: %s", opts->charmap);
            return 1;
        }
        /* We need to store the charmap name in the hardware configuration.
         * However, the charmap file itself is only used by the UI component
         * and doesn't need to be set to the emulation engine.
         */
        kcm_extract_charmap_name(opts->charmap, charmap_name,
                                 sizeof(charmap_name));
        str_reset(&hw->hw_keyboard_charmap, charmap_name);
    }
    return true;
}

bool emulator_initUserInterface(const AndroidOptions* opts,
                                const UiEmuAgent* uiEmuAgent) {
    user_config_init();

    assert(s_skinConfig != NULL);
    assert(s_skinPath != NULL);

    return ui_init(s_skinConfig, s_skinPath, opts, uiEmuAgent);
}

// TODO(digit): Remove once QEMU2 uses emulator_initUserInterface().
bool ui_init(const AConfig* skinConfig,
             const char* skinPath,
             const AndroidOptions* opts,
             const UiEmuAgent* uiEmuAgent) {
    int win_x, win_y;

    signal(SIGINT, SIG_DFL);
#ifndef _WIN32
    signal(SIGQUIT, SIG_DFL);
#endif

    if (opts->ui_only) {
        bool ret_OK = false;

        *(emulator_window_get()->opts) = *opts;
        if (!strcmp("snapshot-control", opts->ui_only)) {
            int winsys_ret;
            winsys_ret = skin_winsys_snapshot_control_start();
            ret_OK = (winsys_ret == 0);
        } else {
            derror("Unknown \"-ui-only\" feature: \"%s\"", opts->ui_only);
        }
        return ret_OK;
    }

    skin_winsys_start(opts->no_window);

    if (opts->no_window) {
#ifndef _WIN32
        /* prevent SIGTTIN and SIGTTOUT from stopping us. this is necessary to
         * be able to run the emulator in the background (e.g. "emulator &").
         * despite the fact that the emulator should not grab input or try to
         * write to the output in normal cases, we're stopped on some systems
         * (e.g. OS X)
         */
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
#endif
#ifdef __APPLE__
        /** Make sure we don't accidentally display an icon b/156675899 */
        ProcessSerialNumber psn = {0, kCurrentProcess};
        TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif
        /** Do not load emulator icon for embedded emulator. */
    } else if (!opts->qt_hide_window) {
        // NOTE: On Windows, the program icon is embedded as a resource inside
        //       the executable. However, this only changes the icon that
        //       appears with the executable in a file browser. To change the
        //       icon that appears both in the application title bar and the
        //       taskbar, the window icon still must be set.
#if defined(__APPLE__) || defined(_WIN32)
        static const char kIconFile[] = "emulator_icon_256.png";
#else
        static const char kIconFile[] = "emulator_icon_128.png";
#endif
        size_t icon_size;
        const unsigned char* icon_data =
                android_emulator_icon_find(kIconFile, &icon_size);

        if (icon_data) {
            skin_winsys_set_window_icon(icon_data, icon_size);
        } else {
            derror("Could not find emulator icon resource: %s", kIconFile);
        }
    }

    user_config_get_window_pos(&win_x, &win_y);

    if (emulator_window_init(emulator_window_get(), skinConfig, skinPath, win_x,
                             win_y, opts, uiEmuAgent) < 0) {
        derror("Could not load emulator skin from '%s'", skinPath);
        return false;
    }

    /* add an onion overlay image if needed */
    if (opts->onion) {
        SkinImage* onion = skin_image_find_simple(opts->onion);
        int alpha, rotate;

        if (opts->onion_alpha && 1 == sscanf(opts->onion_alpha, "%d", &alpha)) {
            alpha = (256 * alpha) / 100;
        } else
            alpha = 128;

        if (opts->onion_rotation &&
            1 == sscanf(opts->onion_rotation, "%d", &rotate)) {
            rotate &= 3;
        } else
            rotate = SKIN_ROTATION_0;

        emulator_window_get()->onion = onion;
        emulator_window_get()->onion_alpha = alpha;
        emulator_window_get()->onion_rotation = rotate;
    }

    return true;
}

void emulator_finiUserInterface(void) {
    if (s_skinConfig) {
        aconfig_node_free(s_skinConfig);
        free(s_skinPath);
        s_skinConfig = NULL;
        s_skinPath = NULL;
    }

    ui_done();
}

// TODO(digit): Remove once QEMU2 uses emulator_finiUserInterface().
void ui_done(void) {
    user_config_done();
    emulator_window_done(emulator_window_get());
    skin_winsys_destroy();
}
