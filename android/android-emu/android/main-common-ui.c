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

#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "android/boot-properties.h"
#include "aemu/base/gl_object_counter.h"
#include "aemu/base/logging/CLog.h"
#include "android/avd/util.h"
#include "android/console.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulator-window.h"
#include "android/main-emugl.h"
#include "android/opengles.h"
#include "android/resource.h"
#include "android/skin/image.h"
#include "android/skin/rect.h"
#include "android/skin/user-config.h"
#include "android/skin/winsys.h"
#include "android/user-config.h"
#include "android/utils/debug.h"
#include "android/utils/string.h"
#include "host-common/feature_control.h"
#include "android/utils/system.h"
#include "host-common/crash-handler.h"
#include "host-common/misc.h"
#include "host-common/multi_display_agent.h"
#include "host-common/vm_operations.h"

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

bool emulator_initMinimalSkinConfig(int lcd_width,
                                    int lcd_height,
                                    const SkinRect* rect,
                                    AUserConfig** userConfig_out);

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

bool emulator_parseInputCommandLineOptions(AndroidOptions* opts) {
#ifndef CONFIG_HEADLESS
    if (opts->use_keycode_forwarding ||
        feature_is_enabled(kFeature_KeycodeForwarding)) {
        getConsoleAgents()->settings->set_keycode_forwading(true);
    }

    // Set keyboard layout for Android only.
    if (getConsoleAgents()->settings->use_keycode_forwarding() &&
        !opts->fuchsia) {
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

bool emulator_initUserInterface(const AndroidOptions* opts,
                                const UiEmuAgent* uiEmuAgent) {
    user_config_init();

    assert(get_skin_config() != NULL);
    assert(get_skin_path() != NULL);

    return ui_init(get_skin_config(), get_skin_path(), opts, uiEmuAgent);
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
    if (get_skin_config()) {
        aconfig_node_free(get_skin_config());
    }

    ui_done();
}

// TODO(digit): Remove once QEMU2 uses emulator_finiUserInterface().
void ui_done(void) {
    user_config_done();
    emulator_window_done(emulator_window_get());
    skin_winsys_destroy();
}

static RendererConfig lastRendererConfig;

static bool isGuestRendererChoice(const char* choice) {
    return choice && (!strcmp(choice, "off") || !strcmp(choice, "guest"));
}

static const char DEFAULT_SOFTWARE_GPU_MODE[] = "swiftshader_indirect";

bool configAndStartRenderer(enum WinsysPreferredGlesBackend uiPreferredBackend,
                            RendererConfig* config_out) {
    EmuglConfig config;
    AvdInfo* avd = getConsoleAgents()->settings->avdInfo();
    AndroidHwConfig* hw = getConsoleAgents()->settings->hw();
    AndroidOptions* opts =
            getConsoleAgents()->settings->android_cmdLineOptions();

    const QAndroidVmOperations* vm_operations = getConsoleAgents()->vm;
    const struct QAndroidEmulatorWindowAgent* window_agent =
            getConsoleAgents()->emu;
    const QAndroidMultiDisplayAgent* multi_display_agent =
            getConsoleAgents()->multi_display;

    int api_level = avdInfo_getApiLevel(avd);
    char* api_arch = avdInfo_getTargetAbi(avd);
    bool isGoogle = avdInfo_isGoogleApis(avd);

    if (avdInfo_sysImgGuestRenderingBlacklisted(avd) &&
        (isGuestRendererChoice(opts->gpu) ||
         isGuestRendererChoice(hw->hw_gpu_mode))) {
        dwarning(
                "Your AVD has been configured with an in-guest renderer, "
                "but the system image does not support guest rendering."
                "Falling back to 'swiftshader_indirect' mode.");
        if (opts->gpu) {
            str_reset(&opts->gpu, DEFAULT_SOFTWARE_GPU_MODE);
        } else {
            str_reset(&hw->hw_gpu_mode, DEFAULT_SOFTWARE_GPU_MODE);
        }
    }

    // Map the generic "software" setting to our default software renderer
    if (!strcmp(hw->hw_gpu_mode, "software")) {
        str_reset(&hw->hw_gpu_mode, DEFAULT_SOFTWARE_GPU_MODE);
    }

    bool hostGpuVulkanBlacklisted = true;

    if (!androidEmuglConfigInit(
                &config, opts->avd, api_arch, api_level, isGoogle, opts->gpu,
                &hw->hw_gpu_mode, 0,
                getConsoleAgents()->settings->host_emulator_is_headless(),
                uiPreferredBackend, &hostGpuVulkanBlacklisted,
                opts->use_host_vulkan)) {
        derror("%s", config.status);
        config_out->openglAlive = 0;

        crashhandler_append_message_format("androidEmuglConfigInit failed.\n");

        lastRendererConfig = *config_out;
        AFREE(api_arch);

        return false;
    }

    // Also write the selected renderer.
    config_out->selectedRenderer = emuglConfig_get_current_renderer();

    // Determine whether to enable Vulkan (if in android qemu mode)

    if (getConsoleAgents()->settings->android_qemu_mode()) {
        // Always enable GLDirectMem for API >= 29
        bool shouldEnableGLDirectMem = api_level >= 29;
#if defined(__aarch64__) && defined(__APPLE__)
        // b/273985153
        shouldEnableGLDirectMem = false;
#endif
        bool shouldEnableVulkan = true;

        crashhandler_append_message_format(
                "Deciding if GLDirectMem/Vulkan should be enabled. "
                "Selected renderer: %d "
                "API level: %d host GPU blacklisted? %d\n",
                config_out->selectedRenderer, api_level,
                hostGpuVulkanBlacklisted);
        switch (config_out->selectedRenderer) {
            // Host gpu: enable as long as not on blacklist
            // and api >= 29
            case SELECTED_RENDERER_HOST:
                shouldEnableVulkan =
                        (api_level >= 29) && !hostGpuVulkanBlacklisted;
                if (shouldEnableVulkan) {
                    crashhandler_append_message_format(
                            "Host GPU selected, enabling Vulkan.\n");
                } else {
                    crashhandler_append_message_format(
                            "Host GPU selected, not enabling Vulkan because "
                            "either API level is < 29 or host GPU driver is "
                            "blacklisted.\n");
                }
                break;
            // Swiftshader: always enable if api level >= 29
            case SELECTED_RENDERER_SWIFTSHADER_INDIRECT:
                shouldEnableVulkan = api_level >= 29;
                if (shouldEnableVulkan) {
                    crashhandler_append_message_format(
                            "Swiftshader selected, enabling Vulkan.\n");
                } else {
                    crashhandler_append_message_format(
                            "Swiftshader selected, not enabling Vulkan because "
                            "API level is < 29.\n");
                }
                break;
            // Other renderers (such as angle, mesa):
            default:
                shouldEnableVulkan = false;
                crashhandler_append_message_format(
                        "Some other renderer selected, not enabling Vulkan.\n");
        }

        if (shouldEnableGLDirectMem) {
            crashhandler_append_message_format("Enabling GLDirectMem");
            // Do not enable if we did not enable it on the API 29 image itself.
            feature_set_if_not_overridden_or_guest_disabled(
                    kFeature_GLDirectMem, true);
        }

        if (shouldEnableVulkan) {
            crashhandler_append_message_format("Enabling Vulkan");
            feature_set_if_not_overridden(kFeature_Vulkan, true);
        } else {
            crashhandler_append_message_format(
                    "Not enabling Vulkan here "
                    "(feature flag may be turned on manually)");
        }
    }

    AFREE(api_arch);

    // If the user is using -gpu off (not -gpu guest),
    // or if the API level is lower than 14 (Ice Cream Sandwich)
    // force 16-bit color depth.

    if (api_level < 14 || (opts->gpu && !strcmp(opts->gpu, "off"))) {
        hw->hw_lcd_depth = 16;
    }

    hw->hw_gpu_enabled = config.enabled;

    /* Update hw_gpu_mode with the canonical renderer name determined by
     * emuglConfig_init (host/guest/off/swiftshader etc)
     */
    str_reset(&hw->hw_gpu_mode, config.backend);
    D("%s", config.status);

    const char* gpu_mode = opts->gpu ? opts->gpu : hw->hw_gpu_mode;

#ifdef _WIN32
    // BUG: https://code.google.com/p/android/issues/detail?id=199427
    // Booting will be severely slowed down, if not disabled outright, when
    // 1. On Windows
    // 2. Using an AVD resolution of >= 1080p (can vary across host setups)
    // 3. -gpu mesa
    // What happens is that Mesa will hog the CPU, while disallowing
    // critical boot services from making progress, causing
    // the services to give up and put the emulator in a reboot loop
    // until it either fails to boot altogether or gets lucky and
    // successfully boots.
    // This workaround disables the boot animation under the above conditions,
    // which frees up the CPU enough for the device to boot.
    if (gpu_mode && (!strcmp(gpu_mode, "mesa"))) {
        opts->no_boot_anim = 1;
        D("Starting AVD without boot animation.\n");
    }
#endif

    emuglConfig_setupEnv(&config);

    // Now start the renderer, if applicable, and determine various things such
    // as max supported GLES version and the correct props to write.
    config_out->glesMode = kAndroidGlesEmulationHost;
    config_out->openglAlive = 1;
    int gles_major_version = 2;
    int gles_minor_version = 0;
    if (strcmp(gpu_mode, "guest") == 0 || strcmp(gpu_mode, "off") == 0 ||
        !hw->hw_gpu_enabled) {
        android_gl_object_counter_get();

        if (avdInfo_isGoogleApis(avd) && avdInfo_getApiLevel(avd) >= 19) {
            config_out->glesMode = kAndroidGlesEmulationGuest;
        } else {
            config_out->glesMode = kAndroidGlesEmulationOff;
        }
    } else {
        int gles_init_res = android_initOpenglesEmulation();
        int renderer_startup_res = android_startOpenglesRenderer(
                hw->hw_lcd_width, hw->hw_lcd_height,
                avdInfo_getAvdFlavor(avd) == AVD_PHONE,
                avdInfo_getApiLevel(avd), vm_operations, window_agent,
                multi_display_agent, &gles_major_version, &gles_minor_version);
        if (gles_init_res || renderer_startup_res) {
            config_out->openglAlive = 0;
            if (gles_init_res) {
                crashhandler_append_message_format(
                        "android_initOpenglesEmulation failed. "
                        "Error: %d\n",
                        gles_init_res);
            }
            if (renderer_startup_res) {
                crashhandler_append_message_format(
                        "android_startOpenglesRenderer failed. "
                        "Error: %d\n",
                        renderer_startup_res);
            }
        } else {
            VERBOSE_INFO(init, "Setting vsync to %d hz", hw->hw_lcd_vsync);
            android_setVsyncHz(hw->hw_lcd_vsync);
        }
    }

    // We need to know boot property
    // for opengles version in advance.
    const bool guest_es3_is_ok =
            feature_is_enabled(kFeature_GLESDynamicVersion);
    if (guest_es3_is_ok) {
        config_out->bootPropOpenglesVersion =
                gles_major_version << 16 | gles_minor_version;
    } else {
        config_out->bootPropOpenglesVersion = (2 << 16) | 0;
    }

    // Now estimate the GL framebuffer size.
    // Use the conservative value for bytes per pixel (RGBA8)
    uint64_t pixelSizeBytes = 4;

    config_out->glFramebufferSizeBytes =
            hw->hw_lcd_width * hw->hw_lcd_height * pixelSizeBytes;

    lastRendererConfig = *config_out;
    return true;
}

RendererConfig getLastRendererConfig(void) {
    return lastRendererConfig;
}

void stopRenderer(void) {
    android_stopOpenglesRenderer(true);
}
