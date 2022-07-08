/*
 * Copyright 2021 Google LLC

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#pragma once

#define _STR(str) #str
#define _XSTR(str) _STR(str)

#define USBASSIST_MAJOR_VERSION 1
#define USBASSIST_MINOR_VERSION 1

#define USBASSIST_VERSION ((USBASSIST_MAJOR_VERSION << 16) | USBASSIST_MINOR_VERSION)

#define USBASSIST_RC_VERSION USBASSIST_MAJOR_VERSION,USBASSIST_MINOR_VERSION
#define USBASSIST_RC_VERSION_STR _XSTR(USBASSIST_MAJOR_VERSION) "." _XSTR(USBASSIST_MINOR_VERSION) "\0"
