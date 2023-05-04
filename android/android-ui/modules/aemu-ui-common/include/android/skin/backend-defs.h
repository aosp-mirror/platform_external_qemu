
/* Copyright (C) 2022 The Android Open Source Project
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DECL_WinsysPreferredGlesBackend
#define DECL_WinsysPreferredGlesBackend
enum WinsysPreferredGlesBackend {
    WINSYS_GLESBACKEND_PREFERENCE_AUTO = 0,
    WINSYS_GLESBACKEND_PREFERENCE_ANGLE = 1,
    WINSYS_GLESBACKEND_PREFERENCE_ANGLE9 = 2,
    WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER = 3,
    WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL = 4,
    WINSYS_GLESBACKEND_PREFERENCE_NUM = 5,
};

enum WinsysPreferredGlesApiLevel {
    WINSYS_GLESAPILEVEL_PREFERENCE_AUTO = 0,
    WINSYS_GLESAPILEVEL_PREFERENCE_MAX = 1,
    WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT = 2,
    WINSYS_GLESAPILEVEL_PREFERENCE_NUM = 3,
};
#endif 

#ifdef __cplusplus
}
#endif
