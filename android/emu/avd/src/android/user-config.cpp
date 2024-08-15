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
#include "android/user-config.h"

#include "aemu/base/memory/ScopedPtr.h"

#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "android/utils/path.h"
#include <stdlib.h>
#include <errno.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

#define  D(...)   VERBOSE_PRINT(init,__VA_ARGS__)

#if 0 /* set to 1 for more debugging */
#  define  DD(...)  D(__VA_ARGS__)
#else
#  define  DD(...)  ((void)0)
#endif

struct AUserConfig {
    ABool        changed;
    int          windowX;
    int          windowY;
    double       windowScale;
    int resizableConfigId;
    int extendedControlsX;
    int extendedControlsY;

    int extenedControlsVerticalAnchor;
    int extenedControlsHorizontalAnchor;
    ABool hasExtendedControlPos;
    uint64_t     uuid;
    int posture;  // only applicable to foldable
    char*        iniPath;
};

/* Name of the user-config file */
#define  USER_CONFIG_FILE  "emulator-user.ini"

#define KEY_POSTURE "posture"
#define  KEY_WINDOW_X  "window.x"
#define  KEY_WINDOW_Y  "window.y"
#define  KEY_WINDOW_SCALE  "window.scale"
#define KEY_RESIZABLE_CONFIG_ID "resizable.config.id"
#define KEY_EXTENDED_CONTROLS_X "extended_controls.x"
#define KEY_EXTENDED_CONTROLS_Y "extended_controls.y"
#define KEY_EXTENDED_CONTROLS_HOR "extended_controls.hanchor"
#define KEY_EXTENDED_CONTROLS_VER "extended_controls.vanchor"
#define  KEY_UUID      "uuid"

#define  DEFAULT_X 100
#define  DEFAULT_Y 100
#define  DEFAULT_SCALE -1.0
#define DEFAULT_RESIZABLE_CONFIG_ID -1

constexpr int DEFAULT_POSTURE_UNKNOWN = 0;

void auserConfig_free( AUserConfig* uconfig) {
    if (uconfig->iniPath) {
        free(uconfig->iniPath);
    }
    delete uconfig;
}

AUserConfig*
auserConfig_new_custom(
    AvdInfo* info,
    const SkinRect* monitorRect,
    int screenWidth, int screenHeight ) {

    std::unique_ptr<AUserConfig> uc(new AUserConfig());
    memset(uc.get(), 0, sizeof(*uc));
    int default_w = (monitorRect->size.w * 3) / 4;
    int default_h = (monitorRect->size.h * 3) / 4;
    if (default_w < 100) default_w = 100;
    if (default_h < 100) default_h = 100;
    uc->windowX  = DEFAULT_X;
    uc->windowY  = DEFAULT_Y;
    uc->windowScale  = DEFAULT_SCALE;
    uc->resizableConfigId = DEFAULT_RESIZABLE_CONFIG_ID;
    uc->posture = DEFAULT_POSTURE_UNKNOWN;
    // uc->windowW  = default_w;
    // uc->windowH  = default_h;
    // uc->frameX   = DEFAULT_X;
    // uc->frameY   = DEFAULT_Y;
    // uc->frameW   = default_w;
    // uc->frameH   = default_h;
    uc->changed  = 1;
    uc->uuid = 0;

    return uc.release();
}

/* Create a new AUserConfig object from a given AvdInfo */
AUserConfig*
auserConfig_new( AvdInfo* info, SkinRect* monitorRect, int screenWidth, int screenHeight )
{
    char          inAndroidBuild = avdInfo_inAndroidBuild(info);
    char          needUUID = 1;
    char          temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    CIniFile*     ini = NULL;

    std::unique_ptr<AUserConfig> uc(new AUserConfig());
    memset(uc.get(), 0, sizeof(*uc));

    /* If we are in the Android build system, store the configuration
     * in ~/.android/emulator-user.ini. otherwise, store it in the file
     * emulator-user.ini in the AVD's content directory.
     */
    if (inAndroidBuild) {
        p = bufprint_config_file(temp, end, USER_CONFIG_FILE);
    } else {
        p = bufprint(temp, end, "%s" PATH_SEP "%s", avdInfo_getContentPath(info),
                     USER_CONFIG_FILE);
    }

    /* handle the unexpected */
    if (p >= end) {
        /* Hmmm, something is weird, let's use a temporary file instead */
        p = bufprint_temp_file(temp, end, USER_CONFIG_FILE);
        if (p >= end) {
            derror("Weird: Cannot create temporary user-config file?");
            return NULL;
        }
        dwarning("Weird: Content path too long, using temporary user-config.");
    }

    uc->iniPath = ASTRDUP(temp);
    DD("looking user-config in: %s", uc->iniPath);

    {
        /* ensure that the parent directory exists */
        android::base::ScopedCPtr<char> parentPath(path_parent(uc->iniPath, 1));
        if (parentPath == NULL) {
            derror("Weird: Can't find parent of user-config file: %s",
                   uc->iniPath);
            return NULL;
        }

        if (!path_exists(parentPath.get())) {
            if (!inAndroidBuild) {
                derror("Weird: No content path for this AVD: %s",
                       parentPath.get());
                return NULL;
            }
            DD("creating missing directory: %s", parentPath.get());
            if (path_mkdir_if_needed_no_cow(parentPath.get(), 0755) < 0) {
                derror("Using empty user-config, can't create %s: %s",
                       parentPath.get(), strerror(errno));
                return NULL;
            }
        }
    }

    if (path_exists(uc->iniPath)) {
        DD("reading user-config file");
        ini = iniFile_newFromFile(uc->iniPath);
        if (ini == NULL) {
            dwarning("Can't read user-config file: %s\nUsing default values",
                     uc->iniPath);
        }
    }
    // Set the default width and height to 3/4 the monitor size
    int default_w = (monitorRect->size.w * 3) / 4;
    int default_h = (monitorRect->size.h * 3) / 4;
    if (default_w < 100) default_w = 100;
    if (default_h < 100) default_h = 100;
    if (screenWidth > 0 && screenHeight > 0) {
        // Reduce one side of the default size to make it match the device's aspect ratio
        double defaultSizeAspectRatio = ((double)default_w)   / default_h;
        double deviceAspectRatio      = ((double)screenWidth) / screenHeight;
        if (defaultSizeAspectRatio < deviceAspectRatio) {
            // Reduce the height
            default_h = default_w / deviceAspectRatio;
        } else {
            // Reduce the width
            default_w = default_h * deviceAspectRatio;
        }
    }
    if (ini != NULL) {
        uc->windowX = iniFile_getInteger(ini, KEY_WINDOW_X, DEFAULT_X);
        DD("    found %s = %d", KEY_WINDOW_X, uc->windowX);

        uc->windowY = iniFile_getInteger(ini, KEY_WINDOW_Y, DEFAULT_Y);
        DD("    found %s = %d", KEY_WINDOW_Y, uc->windowY);
        uc->windowScale = iniFile_getDouble(ini, KEY_WINDOW_SCALE, DEFAULT_SCALE);
        uc->resizableConfigId = iniFile_getInteger(ini, KEY_RESIZABLE_CONFIG_ID,
                                                   DEFAULT_RESIZABLE_CONFIG_ID);
        if (iniFile_hasKey(ini, KEY_EXTENDED_CONTROLS_X)) {
            uc->hasExtendedControlPos = 1;
            uc->extendedControlsX =
                    iniFile_getInteger(ini, KEY_EXTENDED_CONTROLS_X, 0);
            DD("    found %s = %d\n", KEY_EXTENDED_CONTROLS_X,
               uc->extendedControlsX);
        }

        uc->posture =
                iniFile_getInteger(ini, KEY_POSTURE, DEFAULT_POSTURE_UNKNOWN);

        if (iniFile_hasKey(ini, KEY_EXTENDED_CONTROLS_Y)) {
            uc->hasExtendedControlPos = 1;
            uc->extendedControlsY =
                    iniFile_getInteger(ini, KEY_EXTENDED_CONTROLS_Y, 0);
            DD("    found %s = %d", KEY_EXTENDED_CONTROLS_Y,
               uc->extendedControlsY);

        }
        if (iniFile_hasKey(ini, KEY_EXTENDED_CONTROLS_HOR)) {
            uc->extenedControlsHorizontalAnchor =
                    iniFile_getInteger(ini, KEY_EXTENDED_CONTROLS_HOR, 0);
            DD("    found %s = %d", KEY_EXTENDED_CONTROLS_HOR,
               uc->extenedControlsHorizontalAnchor);
        }
        if (iniFile_hasKey(ini, KEY_EXTENDED_CONTROLS_VER)) {
            uc->extenedControlsHorizontalAnchor =
                    iniFile_getInteger(ini, KEY_EXTENDED_CONTROLS_VER, 0);
            DD("    found %s = %d", KEY_EXTENDED_CONTROLS_VER,
               uc->extenedControlsVerticalAnchor);
        }

        if (iniFile_hasKey(ini, KEY_UUID)) {
            uc->uuid = (uint64_t) iniFile_getInt64(ini, KEY_UUID, 0LL);
            needUUID = 0;
            DD("    found %s = %lld", KEY_UUID, uc->uuid);
        }

        iniFile_free(ini);
    }
    else {
        uc->windowX  = DEFAULT_X;
        uc->windowY  = DEFAULT_Y;
        uc->windowScale  = DEFAULT_SCALE;
        uc->resizableConfigId = DEFAULT_RESIZABLE_CONFIG_ID;
        uc->posture = DEFAULT_POSTURE_UNKNOWN;
        uc->changed  = 1;
    }

    /* Generate a 64-bit UUID if necessary. We simply take the
     * current time, which avoids any privacy-related value.
     */
    if (needUUID) {
        struct timeval  tm;

        gettimeofday( &tm, NULL );
        uc->uuid    = (uint64_t)tm.tv_sec*1000 + tm.tv_usec/1000;
        uc->changed = 1;
        DD("    Generated UUID = %lld", uc->uuid);
    }

    return uc.release();
}


uint64_t
auserConfig_getUUID( AUserConfig*  uconfig )
{
    return uconfig->uuid;
}

int auserConfig_getPosture(AUserConfig* uconfig) {
    return uconfig->posture;
}

void auserConfig_setPosture(AUserConfig* uconfig, int posture) {
    if (uconfig->posture != posture) {
        uconfig->posture = posture;
        uconfig->changed = 1;
    }
}

double auserConfig_getWindowScale(AUserConfig* uconfig) {
    return uconfig->windowScale;
}

void auserConfig_setWindowScale(AUserConfig* uconfig, double scale) {
    if (uconfig->windowScale!= scale) {
        uconfig->windowScale = scale;
        uconfig->changed = 1;
    }
}

int auserConfig_getResizableConfig(AUserConfig* uconfig) {
    return uconfig->resizableConfigId;
}

void auserConfig_setResizableConfig(AUserConfig* uconfig, int configid) {
    if (uconfig->resizableConfigId != configid) {
        uconfig->resizableConfigId = configid;
        uconfig->changed = 1;
    }
}

void
auserConfig_getWindowPos( AUserConfig*  uconfig, int  *pX, int  *pY )
{
    *pX = uconfig->windowX;
    *pY = uconfig->windowY;
}


void
auserConfig_setWindowPos( AUserConfig*  uconfig, int  x, int  y )
{
    if (x != uconfig->windowX || y != uconfig->windowY) {
        uconfig->windowX = x;
        uconfig->windowY = y;
        uconfig->changed = 1;
    }
}

int auserConfig_getExtendedControlsPos(AUserConfig* uconfig, int* pX, int* pY, int* hAnchor, int* vAnchor) {
    if (uconfig->hasExtendedControlPos) {
        *pX = uconfig->extendedControlsX;
        *pY = uconfig->extendedControlsY;
        *hAnchor = uconfig->extenedControlsHorizontalAnchor;
        *vAnchor = uconfig->extenedControlsVerticalAnchor;
        return 1;
    } else {
        return 0;
    }
}

int auserConfig_setExtendedControlsPos(AUserConfig* uconfig, int x, int y, int hAnchor, int vAnchor) {
    if (x != uconfig->extendedControlsX || y != uconfig->extendedControlsY) {
        uconfig->extendedControlsX = x;
        uconfig->extendedControlsY = y;
        uconfig->extenedControlsVerticalAnchor = vAnchor;
        uconfig->extenedControlsHorizontalAnchor = hAnchor;
        uconfig->changed = 1;
        uconfig->hasExtendedControlPos = 1;
        return 1;
    } else {
        return 0;
    }
}

/* Save the user configuration back to the content directory.
 * Should be used in an atexit() handler */
void
auserConfig_save( AUserConfig*  uconfig )
{
    CIniFile* ini;

    if (uconfig->changed == 0) {
        D("User-config was not changed.");
        return;
    }

    ini = iniFile_newEmpty(uconfig->iniPath);
    if (ini == NULL) {
        D("Weird: can't allocate user-config iniFile?");
        return;
    }

    // b/356555418: Qt6 display coordinate system depends on the DPIs of each display, as well as
    // the arrangement of the displays relative to the primary display. This means that negative
    // coordinates are now valid, which is causing quite some problems in single-monitor setups,
    // or when monitor arrangement changes. Let's just reset the window position back to the default
    // for now.
    // iniFile_setInteger(ini, KEY_WINDOW_X, uconfig->windowX);
    // iniFile_setInteger(ini, KEY_WINDOW_Y, uconfig->windowY);
    // iniFile_setDouble(ini, KEY_WINDOW_SCALE, uconfig->windowScale);
    iniFile_setInteger(ini, KEY_WINDOW_X, DEFAULT_X);
    iniFile_setInteger(ini, KEY_WINDOW_Y, DEFAULT_Y);
    iniFile_setDouble(ini, KEY_WINDOW_SCALE, DEFAULT_SCALE);

    iniFile_setInteger(ini, KEY_RESIZABLE_CONFIG_ID,
                       uconfig->resizableConfigId);
    iniFile_setInteger(ini, KEY_POSTURE, uconfig->posture);

    if (uconfig->hasExtendedControlPos) {
        iniFile_setInteger(ini, KEY_EXTENDED_CONTROLS_X,
                           uconfig->extendedControlsX);
        iniFile_setInteger(ini, KEY_EXTENDED_CONTROLS_Y,
                           uconfig->extendedControlsY);
        iniFile_setInteger(ini, KEY_EXTENDED_CONTROLS_HOR,
                            uconfig->extenedControlsHorizontalAnchor);
        iniFile_setInteger(ini, KEY_EXTENDED_CONTROLS_VER,
                            uconfig->extenedControlsVerticalAnchor);
    }
    iniFile_setInt64(ini, KEY_UUID, uconfig->uuid);
    if (iniFile_saveToFile(ini, uconfig->iniPath) < 0) {
        dwarning("could not save user configuration: %s: %s",
                 uconfig->iniPath, strerror(errno));
    } else {
        D("User configuration saved to %s", uconfig->iniPath);
    }
    iniFile_free(ini);
}
