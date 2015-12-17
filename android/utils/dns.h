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

#pragma once

#include "android/utils/compiler.h"

#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

/* Retrieve DNS servers configured by the host system and place them in a
 * provided |buffer|. Up to |bufferSize| DNS servers are retrieved. On success
 * the number of servers is returned, on failure a negative value is returned.
 */
int get_system_dns_servers(uint32_t* buffer, size_t bufferSize);

/* Parse a string containing a list of DNS servers separated by comma and place
 * the servers into the provided |buffer|. Up to |bufferSize| servers are
 * parsed. The number of servers added to |buffer|, invalid servers will result
 * in warning messages being printed but the function will continue to attempt
 * to parse the next DNS servers in the input.
 */
size_t parse_dns_servers(const char* servers,
                         uint32_t* buffer,
                         size_t bufferSize);

ANDROID_END_HEADER
