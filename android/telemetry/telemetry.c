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
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "android/utils/path.h"
#include "android/utils/ini.h"
#include "android/telemetry/telemetry.h"
#include "android/telemetry/studio_helper.h"
#include <stdio.h>     // fprintf
#include <ctype.h>     // isdigit
#include <errno.h>     // errno
#include <stdlib.h>    // atoi
#include <sys/time.h>  // gettimeofday
#include <time.h>      // strftim, gmtime

/* #ifdef ANDROID_SDK_TOOLS_REVISION */
/* #define VERSION_STRING STRINGIFY(ANDROID_SDK_TOOLS_REVISION) ".0" */
/* #else */
/* #define VERSION_STRING "standalone"  */
/* #endif */                            // <----------------------------------------------------- TODO : RESTORE THIS
#define VERSION_STRING "24.3.1"        // <----------------------------------------------------- TODO : REMOVE  THIS

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(init,__VA_ARGS__)

#define  TELEMETRY_CFG_FILE     "telemetry.cfg"
/******************************************************************************/

AndroidTelemetryConfig telemetry_cfg[1];
static char cfg_path[PATH_MAX] = {0};

/******************************************************************************/

static int today(void)
{
    struct timeval tv = { 0 };    
    if(gettimeofday (&tv, NULL) < 0) {
        E("Cannot get time of event date --- %s\n", strerror(errno));
        return 0;
    }

    struct tm *gmt = gmtime(&tv.tv_sec);
    if(gmt == NULL) {
        E("Cannot get GMT timestamp \n");
        return 0;
    }

    // comparable date format with day resolution XXXXYYZZWW
    char date[9] = { 0 };
    if(strftime (date, sizeof (date), "%Y%m%d", gmt) != (sizeof(date)-1)) {
        E("Cannot get int value of GMT timestamp");
        return 0;
    }
    
    return atoi(date);
}

int android_telemetry_update_studio_prefs(void)
{
    D("Checking for Android Studio updates\n");

    // re-read from studio preferences
    if(get_android_studio_optins((char *) &telemetry_cfg->optin) < 0) {
        W("Failed to update emulator config with Android Studio opt-in preferences\n");
        return -1;
    }

    AFREE(telemetry_cfg->uid);
    if(get_android_studio_installation_id(&telemetry_cfg->uid) < 0) {
        W("Failed to update emulator config with Android Studio uid\n");
        return -1;
    }

    return 0;
}

int android_telemetry_update_emulator_version(void)
{
    if(strncmp(VERSION_STRING, "standalone", 11) == 0) {
        // this will result to no update-checks if we screw up the VERSION_STRING in build
        D("Running developer version of Android Emulator; update checks disabled");
        return 0;
    }
    
    // update current version from memory
    const char *current_revision = VERSION_STRING;
    AFREE(telemetry_cfg->version_current);
    telemetry_cfg->version_current = strdup(current_revision);
    int current = androidTelemetryConfig_EmulatorVersionAsInt(telemetry_cfg->version_current);
    
    // query server for latest emulator revision and update local/saved values
    const char *latest_revision = "25.0.9";  // ALWAYS get_latest_emulator_revision(); // TODO : MAKE THIS CALL
    AFREE(telemetry_cfg->version_latest); 
    telemetry_cfg->version_latest = strdup(latest_revision);
    // free latest_revision string if necessary : TODO
    int latest = androidTelemetryConfig_EmulatorVersionAsInt(telemetry_cfg->version_latest);

    if(current > latest) {
        // pop - up ?
        // disable after first show ?
        // this will show in nice ugly red in studio terminal to get user's attention
        fprintf(stderr, "The Android Emulator version you are running (%s) "
                "is out of date; please consider updating to latest (%s)\n",
                telemetry_cfg->version_current, telemetry_cfg->version_latest);
    }

    return 0;
}

int android_telemetry_update_other(void)
{
    const char *glvendor = "Unknown GL Vendor";  // TODO : GET LIKE DDMS DOES
    AFREE(telemetry_cfg->glvendor);
    telemetry_cfg->glvendor = strdup(glvendor);
    // free glvendor string if necessary  : TODO

    return 0;
}

int android_telemetry_update(void)
{
    if(cfg_path[0] == 0) {
        E("Need to init telemetry first");
        return -1;
    }
    IniFile*  cfg = iniFile_newFromFile(cfg_path);
    if(cfg == NULL) {
        E("Cannot load %s from disk to update", cfg_path);
        return -1;
    }
    
    //    telemetry_cfg->use_date = today();         // <------------------ TODO : RESTORE THIS
    telemetry_cfg->use_date = today() + rand() % 10 + 1;

    if(telemetry_cfg->use_date - telemetry_cfg->refresh_date >= telemetry_cfg->refresh_period) {
        int retval = 0;
        retval |= android_telemetry_update_emulator_version();
        retval |= android_telemetry_update_studio_prefs();
        retval |= android_telemetry_update_other();
        if(retval < 0) {
            D("Failed to update telemetry data");
            // avoid half baked config file updates
            iniFile_free(cfg);
            return -1;
        }
        telemetry_cfg->refresh_date = telemetry_cfg->use_date;
    }
    
    // write update telemetry config to file and save
    if(androidTelemetryConfig_write(telemetry_cfg, cfg) < 0) {
        E("Could not write %s to file", TELEMETRY_CFG_FILE);
        iniFile_free(cfg);
        return -1;
    }
    if (iniFile_saveToFile(cfg, cfg_path) < 0) {
        E("Unable to save %s at %s; Error: %s\n",
          TELEMETRY_CFG_FILE, cfg_path, strerror(errno));
        iniFile_free(cfg);
        return -1;
    }

    iniFile_free(cfg);
    return 0;
}    

/* Read or create new telemetry.cfg file */
int android_telemetry_init()
{
    char *p   =  cfg_path;
    char *end = &cfg_path[PATH_MAX];
    p = bufprint_config_file(cfg_path, end, TELEMETRY_CFG_FILE);
    if ( p >= end ) {
        E("%s cannot be opened", TELEMETRY_CFG_FILE);
        return -1;
    }

    // read telemetry config from saved file, or create a new one
    // with default values if none exists
    IniFile* cfg = iniFile_newFromFile(cfg_path);
    if(cfg == NULL) {
        // init with default values and save to file
        cfg = iniFile_newFromMemory("", NULL);
        if(cfg == NULL ) {
            E("Cannot reset %s; can't allocate new file",
              TELEMETRY_CFG_FILE);
            return -1;
        }
        androidTelemetryConfig_init(telemetry_cfg);
        androidTelemetryConfig_write(telemetry_cfg, cfg);
        if(iniFile_saveToFile(cfg, cfg_path) < 0) {
            E("Cannot initiate %s; can't reset %s file",
              TELEMETRY_CFG_FILE, cfg_path);
            return -1;
        }
    } else {
        cfg = iniFile_newFromFile(cfg_path);
        if(cfg == NULL) {
            E("Cannot load %s from disk to init", cfg_path);
            return -1;
        }
        if(androidTelemetryConfig_read(telemetry_cfg, cfg) < 0) {
            W("Failed to read %s from disk\n", TELEMETRY_CFG_FILE);
            return -1;
        }            
    }
    iniFile_free(cfg);

    androidTelemetryConfig_print(telemetry_cfg);

    return android_telemetry_update();
}

