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
int android_dns_get_system_servers(uint32_t* buffer, size_t bufferSize);

/* Parse a string containing a list of DNS servers separated by comma and place
 * the servers into the provided |buffer|. Up to |bufferSize| servers are
 * parsed. Returns the number of servers added to |buffer| or a negative value
 * on error. Currently the following negative return values have special
 * meaning:
 * -2 Number of servers in |servers| string exceeds |bufferSize|
 */
int android_dns_parse_servers(const char* servers,
                              uint32_t* buffer,
                              size_t bufferSize);

ANDROID_END_HEADER
