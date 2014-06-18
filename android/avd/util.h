/* Copyright (C) 2011 The Android Open Source Project
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
#ifndef _ANDROID_AVD_UTIL_H
#define _ANDROID_AVD_UTIL_H

#include "android/utils/compiler.h"
#include "android/utils/file_data.h"

ANDROID_BEGIN_HEADER

/* A collection of simple functions to extract relevant AVD-related
 * information either from an SDK AVD or a platform build.
 */

/* Return the path to the Android SDK root installation.
 *
 * (*pFromEnv) will be set to 1 if it comes from the $ANDROID_SDK_ROOT
 * environment variable, or 0 otherwise.
 *
 * Caller must free() returned string.
 */
char* path_getSdkRoot( char *pFromEnv );

/* Return the path to the AVD's root configuration .ini file. it is located in
 * ~/.android/avd/<name>.ini or Windows equivalent
 *
 * This file contains the path to the AVD's content directory, which
 * includes its own config.ini.
 */
char* path_getRootIniPath( const char*  avdName );

/* Return the target architecture for a given AVD.
 * Called must free() returned string.
 */
char* path_getAvdTargetArch( const char* avdName );

typedef enum {
    RESULT_INVALID   = -1, // key was found but value contained invalid data
    RESULT_FOUND     =  0, // key was found and value parsed correctly
    RESULT_NOT_FOUND =  1, // key was not found (default used)
} SearchResult;

/* Retrieves an integer value associated with the key parameter
 *
 * |data| is a FileData instance
 * |key| name of key to search for
 * |searchResult| if non-null, this is set to RESULT_INVALID, RESULT_FOUND,
 *                or RESULT_NOT_FOUND
 * Returns valid parsed int value if found, |default| otherwise
 */
int propertyFile_getInt(const FileData* data, const char* key, int _default,
                        SearchResult* searchResult);

/* Retrieves a string corresponding to the target architecture
 * extracted from a build properties file.
 *
 * |data| is a FileData instance holding the build.prop contents.
 * Returns a a new string that must be freed by the caller, which can
 * be 'armeabi', 'armeabi-v7a', 'x86', etc... or NULL if
 * it cannot be determined.
 */
char* propertyFile_getTargetAbi(const FileData* data);

/* Retrieves a string corresponding to the target architecture
 * extracted from a build properties file.
 *
 * |data| is a FileData instance holding the build.prop contents.
 * Returns a new string that must be freed by the caller, which can
 * be 'arm', 'x86, 'mips', etc..., or NULL if if cannot be determined.
 */
char* propertyFile_getTargetArch(const FileData* data);

/* Retrieve the target API level from the build.prop contents.
 * Returns a very large value (e.g. 100000) if it cannot be determined
 * (which happens for platform builds), or 3 (the minimum SDK API level)
 * if there is invalid value.
 */
int propertyFile_getApiLevel(const FileData* data);

/* Retrieve the mode describing how the ADB daemon is communicating with
 * the emulator from inside the guest.
 * Return 0 for legacy mode, which uses TCP port 5555.
 * Return 1 for the 'qemud' mode, which uses a QEMUD service instead.
 */
int propertyFile_getAdbdCommunicationMode(const FileData* data);

/* Return the path of the build properties file (build.prop) from an
 * Android platform build, or NULL if it doesn't exist.
 */
char* path_getBuildBuildProp( const char* androidOut );

/* Return the path of the boot properties file (boot.prop) from an
 * Android platform build, or NULL if it doesn't exit.
 */
char* path_getBuildBootProp( const char* androidOut );

/* Return the target architecture from the build properties file
 *  (build.prop) of an Android platformn build. Return NULL or a new
 * string that must be freed by the caller.
 */
char* path_getBuildTargetArch( const char* androidOut );

/* Given an AVD's target architecture, return the suffix of the
 * corresponding emulator backend program, e.g. 'x86' for x86 and x86_64
 * CPUs, 'arm' for ARM ones (including ARM64 when support is complete),
 * etc. Returned string must not be freed by the caller.
 */
const char* emulator_getBackendSuffix(const char* targetArch);

ANDROID_END_HEADER

#endif /* _ANDROID_AVD_UTIL_H */
