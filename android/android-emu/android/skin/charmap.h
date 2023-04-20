/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/skin/keycode.h"         // Keycodes
#include "android/skin/keycode-buffer.h"  // for SkinKeycodeBuffer
#include "android/utils/compiler.h"       // for ANDROID_BEGIN_HEADER, ANDRO...

ANDROID_BEGIN_HEADER

/* this defines a structure used to describe an Android keyboard charmap */
typedef struct SkinKeyEntry {
    unsigned short  code;
    unsigned short  base;
    unsigned short  caps;
    unsigned short  fn;
    unsigned short  caps_fn;
    unsigned short  number;
} SkinKeyEntry;

/* Defines size of name buffer in SkinCharmap entry. */
#define SKIN_CHARMAP_NAME_SIZE   32

typedef struct SkinCharmap {
    const SkinKeyEntry*  entries;
    int num_entries;
    char name[SKIN_CHARMAP_NAME_SIZE];
} SkinCharmap;

/* Extracts charmap name from .kcm file name.
 * Charmap name, extracted by this routine is a name of the kcm file, trimmed
 * of file name extension, and shrinked (if necessary) to fit into the name
 * buffer. Here are examples on how this routine extracts charmap name:
 * /a/path/to/kcmfile.kcm       -> kcmfile
 * /a/path/to/kcmfile.ext.kcm   -> kcmfile.ext
 * /a/path/to/kcmfile           -> kcmfile
 * /a/path/to/.kcmfile          -> kcmfile
 * /a/path/to/.kcmfile.kcm      -> .kcmfile
 * kcm_file_path - Path to key charmap file to extract charmap name from.
 * charmap_name - Buffer, where to save extracted charname.
 * max_len - charmap_name buffer size.
*/
void kcm_extract_charmap_name(const char* kcm_file_path,
                              char* charmap_name,
                              int max_len);

/* Parse a charmap file and add it to our list.
 * Key charmap array always contains two maps: one for qwerty, and
 * another for qwerty2 keyboard layout. However, a custom layout can
 * be requested with -charmap option. In tha case kcm_file_path
 * parameter contains path to a .kcm file that defines that custom
 * layout, and as the result, key charmap array will contain another
 * entry built from that file. If -charmap option was not specified,
 * kcm_file_path is NULL and final key charmap array will contain only
 * two default entries.
 * Returns a zero value on success, or -1 on failure.
 *
 * Note: on success, the charmap will be returned by skin_charmap_get()
 */
int skin_charmap_setup(const char* kcm_file_path);

/* Cleanups initialization performed in skin_charmap_setup routine. */
void skin_charmap_done(void);

/* Gets charmap descriptor by its name.
 * This routine tries to find a charmap by name. This will compare the
 * name to the default charmap's name, or any charmap loaded with
 * skin_charmap_setup(). Returns NULL on failure.
 */
const SkinCharmap* skin_charmap_get_by_name(const char* name);

/* Maps given unicode key character into a keycode and adds mapped keycode into
 * keycode array. This routine uses charmap passed as cmap parameter to do the
 * translation, and 'down' parameter to generate appropriate ('down' or 'up')
 * keycode.
 */
int
skin_charmap_reverse_map_unicode(const SkinCharmap* cmap,
                                 unsigned int unicode,
                                 int  down,
                                 SkinKeycodeBuffer* keycodes);

/* Return a pointer to the active charmap. If skin_charmap_setup() was
 * called succesfully, this corresponds to the newly loaded charmap.
 *
 * Otherwise, return a pointer to the default charmap.
 */
const SkinCharmap* skin_charmap_get(void);

ANDROID_END_HEADER
