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

#include <string.h>
#include <windows.h>

SharedLibrary* shared_library_open(const char *name, Error **error) {
    GString* libpath = g_string_new(name);

    /* Append a .dll suffix to the library name if there isn't one */
    const char* basename = strrchr(name, '\\');
    const char* basename2 = strrchr(name, '/');
    if (!basename || (basename2 && basename2 > basename)) {
        basename = basename2;
    }
    if (!basename) {
        basename = name;
    }
    if (!strchr(basename, '.')) {
        g_string_append(libpath, ".dll");
    }

    HMODULE lib = LoadLibrary(libpath->str);
    if (!lib) {
        char *pstr;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      NULL,
                      GetLastError(),
                      0,
                      (LPTSTR)&pstr,
                      2,
                      NULL);
        error_setg(error, "Could not open library %s [%s]",
                   libpath->str, pstr);
        LocalFree(pstr);
    }
    g_string_free(libpath, TRUE);

    return (SharedLibrary*)lib;
}

void* shared_library_find(SharedLibrary* library, const char* symbol) {
    if (!library) {
        return NULL;
    }
    void* result = GetProcAddress((HMODULE)library, symbol);
    return result;
}

void shared_library_close(SharedLibrary* library) {
    if (library) {
        FreeLibrary((HMODULE)library);
    }
}
