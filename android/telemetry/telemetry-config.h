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

#ifndef _ANDROID_TELEMETRY_TELEMETRY_CONFIG_H
#define _ANDROID_TELEMETRY_TELEMETRY_CONFIG_H

#include <stdint.h>
#include "android/utils/compiler.h"
#include "android/utils/ini.h"

ANDROID_BEGIN_HEADER

typedef char      telemetry_bool_t;
typedef int       telemetry_int_t;
typedef char*     telemetry_string_t;

/* these macros are used to define the fields of AndroidTelemetryConfig
 * declared below
 */
#define   TELEMETRYCFG_BOOL(n,s,d,a,t)       telemetry_bool_t      n;
#define   TELEMETRYCFG_INT(n,s,d,a,t)        telemetry_int_t       n;
#define   TELEMETRYCFG_STRING(n,s,d,a,t)     telemetry_string_t    n;
#define   TELEMETRYCFG_UID(n,s,d,a,t)        telemetry_uid_t       n;

typedef struct {
#include "android/telemetry/telemetry-config-defs.h"
} AndroidTelemetryConfig;

/* Set all default values */
void androidTelemetryConfig_init( AndroidTelemetryConfig*  emuConfig );

/* reads an emulator configuration file from disk
 * returns -1 if the file could not be read, or 0 in case of success.
 *
 * note that default values are written to emuConfig if the configuration
 * file doesn't have the corresponding properties.
 */
int  androidTelemetryConfig_read( AndroidTelemetryConfig*  emuConfig,
                                  IniFile*          cfg );

/* Write an emualtor configuration to a config file object.
 * Returns 0 in case of success. Note that any value that is set to the
 * default will not bet written.
 */
int  androidTelemetryConfig_write( AndroidTelemetryConfig*  emuConfig,
                                   IniFile*          cfg );

/* Finalize a given emulator configuration */
void androidTelemetryConfig_done( AndroidTelemetryConfig* config );

/* Pretty-print android emulator telemetry config file */
void androidTelemetryConfig_print( AndroidTelemetryConfig* config );

/* Get comparable int values of emulator reivision */
int androidTelemetryConfig_EmulatorVersionAsInt(const char *version);


ANDROID_END_HEADER

#endif /* _ANDROID_TELEMETRY_TELEMETRY_CONFIG_H */
