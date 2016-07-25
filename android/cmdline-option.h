/* Copyright (C) 2008 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/* a structure used to model a linked list of parameters
 */
typedef struct ParamList {
    char*              param;
    struct ParamList*  next;
} ParamList;

/* define a structure that will hold all option variables
 */
typedef struct {
#define OPT_LIST(n,t,d)    ParamList*  n;
#define OPT_PARAM(n,t,d)   char*  n;
#define OPT_FLAG(n,d)      int    n;
#include "android/cmdline-options.h"
} AndroidOptions;


/* Parse command-line arguments options and remove them from (argc,argv)
 * 'opt' will be set to the content of parsed options
 * returns 0 on success, -1 on error (unknown option)
 */
extern int
android_parse_options( int  *pargc, char**  *pargv, AndroidOptions*  opt );

/* Parse |port_string| into |console_port| and |adb_port|. Error checking is
 * done and approriate warning or error messages shown if needed. If the
 * function returns false the output parameter is not touched.
 */
extern bool android_parse_port_option(const char* port_string,
                                      int* console_port,
                                      int* adb_port);

/* Parse |ports_string| into |console_port| and |adb_port|. The format of the
 * -ports option is currently "<console port>,<adb port>". Error checking is
 * done and the approriate warning or error messages shown if needed. If the
 * function returns false neither of the output parameters are touched.
 */
extern bool android_parse_ports_option(const char* ports_string,
                                       int* console_port,
                                       int* adb_port);

/* Validates the ports in use and warns the user if they are likely to cause
 * problems. Also announces the serial number of the emulator as seen from adb.
 *
 * Returns: true if no validation errors were detected, false otherwise.
 */
extern bool android_validate_ports(int console_port, int adb_port);

// Parse the debug-related combination of flags from the |opt| argument and
// apply them.
// Expected format: <option>[,<option>[,...]]
//   where <option> is either one of the debug flags or "all" to control
//   all flags at once. If an <option> has a '-' or 'no-' prefix, that flag is
//   disabled. Unknown flags are just ignored. Flags are applied left-to-right,
//   so the last flag value (flag or no-flag) wins.
// E.g. |opt| = "all,-init,no-qemud,blah" enables all debugging flags except
//   of "init" and "qemud". "blah" flag doesn't exist (I hope) so it adds
//   nothing.
//
// returns true if the |opt| had at least one valid debug flag.
bool android_parse_debug_option(const char* opt);

/* the default device DPI if none is specified by the skin
 */
#define  DEFAULT_DEVICE_DPI  165

ANDROID_END_HEADER
