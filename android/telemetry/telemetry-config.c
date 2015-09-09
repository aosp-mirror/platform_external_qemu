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
#include "android/telemetry/telemetry-config.h"
#include "android/utils/ini.h"
#include "android/utils/system.h"
#include "android/telemetry/studio_helper.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    W(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(init,__VA_ARGS__)

static int
stringToBoolean( const char* value )
{
    if (!strcmp(value,"1")    ||
        !strcmp(value,"yes")  ||
        !strcmp(value,"YES")  ||
        !strcmp(value,"true") ||
        !strcmp(value,"TRUE"))
        {
            return 1;
        }
    else
        return 0;
}

void
androidTelemetryConfig_init( AndroidTelemetryConfig*  config)
{
    #define   TELEMETRYCFG_BOOL(n,s,d,a,t)       config->n = stringToBoolean(d);
    #define   TELEMETRYCFG_INT(n,s,d,a,t)        config->n = d;
    #define   TELEMETRYCFG_STRING(n,s,d,a,t)     config->n = ASTRDUP(d);

    #include "android/telemetry/telemetry-config-defs.h"
}

int
androidTelemetryConfig_read( AndroidTelemetryConfig*  config,
                             IniFile*           cfg )
{
    if (cfg == NULL)
        return -1;

    /* use the magic of macros to implement the emulator configuration loading */

    #define   TELEMETRYCFG_BOOL(n,s,d,a,t)       if (iniFile_getValue(cfg, s)) { config->n = iniFile_getBoolean(cfg, s, d); }
    #define   TELEMETRYCFG_INT(n,s,d,a,t)        if (iniFile_getValue(cfg, s)) { config->n = iniFile_getInteger(cfg, s, d); }
    #define   TELEMETRYCFG_STRING(n,s,d,a,t)     if (iniFile_getValue(cfg, s)) { AFREE(config->n); config->n = iniFile_getString(cfg, s, d); }

    #include "android/telemetry/telemetry-config-defs.h"

    return 0;
}

int
androidTelemetryConfig_write( AndroidTelemetryConfig* config,
                              IniFile*           cfg )
{
    if (cfg == NULL)
        return -1;

    /* use the magic of macros to implement the emulator configuration saving */

    #define   TELEMETRYCFG_BOOL(n,s,d,a,t)       iniFile_setBoolean(cfg, s, config->n);
    #define   TELEMETRYCFG_INT(n,s,d,a,t)        iniFile_setInteger(cfg, s, config->n);
    #define   TELEMETRYCFG_STRING(n,s,d,a,t)     iniFile_setValue(cfg, s, config->n);

    #include "android/telemetry/telemetry-config-defs.h"

    return 0;
}

void
androidTelemetryConfig_done( AndroidTelemetryConfig* config )
{
    #define   TELEMETRYCFG_BOOL(n,s,d,a,t)       config->n = 0;
    #define   TELEMETRYCFG_INT(n,s,d,a,t)        config->n = 0;
    #define   TELEMETRYCFG_STRING(n,s,d,a,t)     AFREE(config->n);

    #include "android/telemetry/telemetry-config-defs.h"
}

void
androidTelemetryConfig_print( AndroidTelemetryConfig* config )
{
#define   TELEMETRYCFG_BOOL(n,s,d,a,t)       fprintf(stdout, "* %-16s:%-40s \t (%s) \n", s, config->n != 0 ? "true" : "false", a);
#define   TELEMETRYCFG_INT(n,s,d,a,t)        fprintf(stdout, "* %-16s:%-40d \t (%s) \n", s, config->n, a);
#define   TELEMETRYCFG_STRING(n,s,d,a,t)     fprintf(stdout, "* %-16s:%-40s \t (%s) \n", s, config->n, a);

    #include "android/telemetry/telemetry-config-defs.h"
}

/* Convert android revision number to a comparable int */
int
androidTelemetryConfig_EmulatorVersionAsInt(const char *version)
{
    int retval = 0;
    if(version == NULL) {
        E("Unitialized emulator version string");
        return -1;    
    }

    char *v = strdup(version);
    int i = 0;
    for(i = 0; i < strlen(v); i++) {
        if(isdigit(v[i]))
            continue;
        v[i] = '0';
    }
    retval = atoi(v);
    
    android_free(v);
    return retval;
}
