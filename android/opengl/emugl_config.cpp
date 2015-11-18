// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/opengl/emugl_config.h"

#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/opengl/EmuglBackendList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

#if DEBUG
#define D(...)  printf(__VA_ARGS__)
#else
#define D(...)  ((void)0)
#endif


using android::base::String;
using android::base::StringFormat;
using android::base::StringVector;
using android::base::System;
using android::opengl::EmuglBackendList;

static EmuglBackendList* sBackendList = NULL;

static void resetBackendList(int bitness) {
    delete sBackendList;
    sBackendList = new EmuglBackendList(
            System::get()->getLauncherDirectory().c_str(), bitness);
}

static bool stringVectorContains(const StringVector& list,
                                 const char* value) {
    for (size_t n = 0; n < list.size(); ++n) {
        if (!strcmp(list[n].c_str(), value)) {
            return true;
        }
    }
    return false;
}

bool emuglConfig_init(EmuglConfig* config,
                      bool gpu_enabled,
                      const char* gpu_mode,
                      const char* gpu_option,
                      int bitness,
                      bool no_window) {
    // zero all fields first.
    memset(config, 0, sizeof(*config));

    // The value of '-gpu <mode>' overrides the hardware properties,
    // except if <mode> is 'auto'.
    if (gpu_option) {
        if (!strcmp(gpu_option, "on") || !strcmp(gpu_option, "enable")) {
            gpu_enabled = true;
            if (!gpu_mode || !strcmp(gpu_mode, "auto")) {
                gpu_mode = "host";
            }
        } else if (!strcmp(gpu_option, "off") ||
                   !strcmp(gpu_option, "disable")) {
            gpu_enabled = false;
        } else if (!strcmp(gpu_option, "auto")) {
            // Nothing to do
        } else {
            gpu_enabled = true;
            gpu_mode = gpu_option;
        }
    }

    if (!gpu_enabled) {
        config->enabled = false;
        snprintf(config->status, sizeof(config->status),
                 "GPU emulation is disabled");
        return true;
    }

    if (!bitness) {
        bitness = System::get()->getHostBitness();
    }
    config->bitness = bitness;
    resetBackendList(bitness);

    // Check that the GPU mode is a valid value. 'auto' means determine
    // the best mode depending on the environment. Its purpose is to
    // enable 'mesa' mode automatically when NX or Chrome Remote Desktop
    // is detected.
    if (!strcmp(gpu_mode, "auto") && !gpu_option) {
        // The default will be 'host' unless NX or Chrome Remote Desktop
        // is detected, or |no_window| is true.
        String sessionType;
        if (System::get()->isRemoteSession(&sessionType)) {
            D("%s: %s session detected\n", __FUNCTION__, sessionType.c_str());
            if (!sBackendList->contains("mesa")) {
                config->enabled = false;
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled under %s without Mesa",
                        sessionType.c_str());
                return true;
            }
            D("%s: 'mesa' mode auto-selected\n", __FUNCTION__);
            gpu_mode = "mesa";
        } else if (no_window) {
            if (stringVectorContains(sBackendList->names(), "mesa")) {
                D("%s: Headless (-no-window) mode, using Mesa backend\n",
                  __FUNCTION__);
                gpu_mode = "mesa";
            } else {
                D("%s: Headless (-no-window) mode without Mesa, forcing '-gpu off'\n",
                  __FUNCTION__);
                config->enabled = false;
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled (-no-window without Mesa)");
                return true;
            }
        } else {
            D("%s: 'host' mode auto-selected\n", __FUNCTION__);
            gpu_mode = "host";
        }
    }

    // 'host' is a special value corresponding to the default translation
    // to desktop GL, 'guest' does not use host-side emulation,
    // anything else must be checked against existing host-side backends.
    if (strcmp(gpu_mode, "host") != 0 && strcmp(gpu_mode, "guest") != 0) {
        const StringVector& backends = sBackendList->names();
        if (!stringVectorContains(backends, gpu_mode)) {
            String error = StringFormat(
                "Invalid GPU mode '%s', use one of: on off host guest", gpu_mode);
            for (size_t n = 0; n < backends.size(); ++n) {
                error += " ";
                error += backends[n];
            }
            config->enabled = false;
            snprintf(config->status, sizeof(config->status), "%s",
                     error.c_str());
            return false;
        }
    }
    config->enabled = true;
    snprintf(config->backend, sizeof(config->backend), gpu_mode);
    snprintf(config->status, sizeof(config->status),
             "GPU emulation enabled using '%s' mode", gpu_mode);
    return true;
}

void emuglConfig_setupEnv(const EmuglConfig* config) {
    System* system = System::get();

    if (!config->enabled) {
        // There is no real GPU emulation. As a special case, define
        // SDL_RENDER_DRIVER to 'software' to ensure that the
        // software SDL renderer is being used. This allows one
        // to run with '-gpu off' under NX and Chrome Remote Desktop
        // properly.
        system->envSet("SDL_RENDER_DRIVER", "software");
        return;
    }

    // $EXEC_DIR/<lib>/ is already added to the library search path by default,
    // since generic libraries are bundled there. We may need more though:
    resetBackendList(config->bitness);
    if (strcmp(config->backend, "host") != 0) {
        // If the backend is not 'host', we also need to add the
        // backend directory.
        String dir = sBackendList->getLibDirPath(config->backend);
        if (dir.size()) {
            D("Adding to the library search path: %s\n", newDirs.c_str());
            system->addLibrarySearchDir(dir.c_str());
        }
    }

    if (!strcmp(config->backend, "host")) {
        // Nothing more to do for the 'host' backend.
        return;
    }

    // For now, EmuGL selects its own translation libraries for
    // EGL/GLES libraries, unless the following environment
    // variables are defined:
    //    ANDROID_EGL_LIB
    //    ANDROID_GLESv1_LIB
    //    ANDROID_GLESv2_LIB
    //
    // If a backend provides one of these libraries, use it.
    String lib;
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_EGL, &lib)) {
        system->envSet("ANDROID_EGL_LIB", lib.c_str());
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv1, &lib)) {
        system->envSet("ANDROID_GLESv1_LIB", lib.c_str());
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv2, &lib)) {
        system->envSet("ANDROID_GLESv2_LIB", lib.c_str());
    }

    if (!strcmp(config->backend, "mesa")) {
        system->envSet("ANDROID_GL_LIB", "mesa");
        system->envSet("ANDROID_GL_SOFTWARE_RENDERER", "1");
    }
}
