/* Copyright (C) 2017 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_PSTORE_H
#define _HW_GOLDFISH_PSTORE_H

#define TYPE_GOLDFISH_PSTORE     "goldfish_pstore"
#define GOLDFISH_PSTORE_DEV_ID   "goldfish_pstore"
#if defined(TARGET_MIPS)
#define GOLDFISH_PSTORE_MEM_BASE 0x1f020000
#else
#define GOLDFISH_PSTORE_MEM_BASE 0xff018000
#endif
#define GOLDFISH_PSTORE_MEM_SIZE 0x10000
#define GOLDFISH_PSTORE_ADDR_PROP "addr"
#define GOLDFISH_PSTORE_SIZE_PROP "size"
#define GOLDFISH_PSTORE_FILE_PROP "file"

#ifdef _WIN32
 #include <io.h>
 #include <fcntl.h>
 #define OPEN_WRITE(fname) _open((fname), _O_CREAT|_O_TRUNC|_O_RDWR|_O_BINARY);
 #define OPEN_READ(fname) _open((fname), _O_RDONLY|_O_BINARY);
#else
 #define OPEN_WRITE(fname) open((fname), O_CREAT|O_SYNC|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)
 #define OPEN_READ(fname) open((fname), O_RDONLY);
#endif
#endif /* _HW_GOLDFISH_PSTORE_H */

