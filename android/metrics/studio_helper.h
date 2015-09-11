/* Copyright 2015 The Android Open Source Project
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
#ifndef ANDROID_TELEMETRY_STUDIO_HELPER_H
#define ANDROID_TELEMETRY_STUDIO_HELPER_H

#include "android/utils/compiler.h"
#include <stdint.h>

ANDROID_BEGIN_HEADER

#define STUDIO_OPTINS_LEN_MAX 16
#define STUDIO_UID_LEN_MAX 64

/* This header files describes functions that help the android emulator
 * obtain values stored in AndroidStudio preferences.
 * These values are stored in XML files under well-defined paths
 * that might differ among platforms and Android Studio versions.
 * All functions take strings (char *) as value-return arguments.
 * If one or more installations of AndroidStudio exist,
 * the value-return argument will be updated to coresspond
 * to preferences set in the latest Studio (revision) installation
 * and 0 will be returned. Else, or in the even of a failure,
 * a value < 0 will be  returned
 */

/* This function takes string |retval| as a value-return
 * argument, and assumes  sizeof(*retval) >= STUDIO_OPTINS_LEN_MAX.
 * Upon successful completion, the value "true" (if user has
 * opted in to crash-reporing in Android Studio) or "false"
 * (otherwise) will be stored in |retval and 0 will be returned.
 * In the event of a failure, a value < 0 will be returned.
 */
int android_studio_get_optins(char* retval);

/* This function takes string |retval| as a value-return
 * argument, and assumes  sizeof(*retval) >= STUDIO_UID_LEN_MAX.
 * Upon successful completion, the AndroidStudio installation ID
 * will be copied to |retval| and 0 will be returned.
 * In the event of a failure, a value < 0 will be returned.
 */
int android_studio_get_installation_id(char* retval);

ANDROID_END_HEADER

#endif  // ANDROID_TELEMETRY_STUDIO_HELPER_H
