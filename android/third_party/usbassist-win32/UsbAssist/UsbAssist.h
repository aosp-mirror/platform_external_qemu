/*
 * Copyright 2021 Google LLC
 *
 * This program is ExFreePool software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#pragma once

#define USBASSIST_IOCTL_API_VERSION		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x601, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define USB_PASSTHROUGH_DEV_ADD 1
#define USB_PASSTHROUGH_DEV_DEL 2
#define USB_PASSTHROUGH_ONESHOT 1
typedef struct __USB_PASSTHROUGH_TARGET {
	USHORT vid;
	USHORT pid;
	ULONG  bus;
	UCHAR port[8];
	ULONG flags;
	ULONG ops;
} USB_PASSTHROUGH_TARGET, * PUSB_PASSTHROUGH_TARGET;
#define USBASSIST_IOCTL_DEVICE_OPS		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x602, METHOD_BUFFERED, FILE_WRITE_ACCESS)
