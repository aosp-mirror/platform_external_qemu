/* Copyright 2015 Android Source Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/
#ifndef ANDROID_BOOT_PROPERTIES_H
#define ANDROID_BOOT_PROPERTIES_H

/* Initialize Android boot properties backend. */
void android_boot_properties_init(void);

/* Add one boot property (key, value) pair */
void android_boot_property_add(const char* name, const char* value);

#endif  /* ANDROID_BOOT_PROPERTIES_H */
