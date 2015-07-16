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
#include "qemu/shared-library.h"

#include <glib.h>

#include <dlfcn.h>
#include <string.h>

#ifdef __APPLE__
#  define SO_EXTENSION ".dylib"
#else
#  define SO_EXTENSION ".so"
#endif

SharedLibrary* shared_library_open(const char *name, Error **error) {
    GString* libpath = g_string_new(name);

    /* Append a library file extension to the name if it doesn't have one */
    const char* basename = strrchr(name, '/');
    if (!basename) {
        basename = name;
    }
    if (!strchr(basename, '.')) {
        g_string_append(libpath, SO_EXTENSION);
    }

    dlerror();
    SharedLibrary* result = dlopen(libpath->str, RTLD_LAZY);
    if (!result) {
        error_setg(error, "Could not open library %s [%s]", name, dlerror());
    }
    g_string_free(libpath, TRUE);
    return result;
}

void* shared_library_find(SharedLibrary* library, const char* symbol) {
    if (!library) {
        return NULL;
    }
    return dlsym(library, symbol);
}

void shared_library_close(SharedLibrary* library) {
    if (library) {
        dlclose(library);
    }
}
