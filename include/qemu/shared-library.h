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
#ifndef ANDROID_SHARED_LIBRARY_H
#define ANDROID_SHARED_LIBRARY_H

#include "qapi/error.h"

// Opaque handle to a shared library instance.
typedef struct SharedLibrary SharedLibrary;

// Open a shared library. |name| is either its name or full path.
// On success, return a non-NULL value. On failure, sets |*error| and
// return NULL.
//
// A library can be opened several times, and will only be loaded once
// in the system. However, there is no guarantee that the corresponding
// SharedLibrary* value will be the same for each call.
//
SharedLibrary* shared_library_open(const char* name, Error **error);

// Probe shared |library| for a specific dynamic |symbol| and return its
// address in memory, or NULL if not found.
void* shared_library_find(SharedLibrary* library, const char* symbol);

// Close shared |library| instance. This will unload the library when its
// internal reference count reaches 0.
void shared_library_close(SharedLibrary* library);

#endif  // ANDROID_SHARED_LIBRARY_H
