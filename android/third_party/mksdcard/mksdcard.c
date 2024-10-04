/* mksdcard.c
**
** Copyright 2007, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* a simple and portable program used to generate a blank FAT32 image file
 *
 * usage: mksdcard [-l label] <size> <filename>
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#endif

/* believe me, you *don't* want to change these constants !! */
#define  BYTES_PER_SECTOR    512
#define  RESERVED_SECTORS    32
#define  BACKUP_BOOT_SECTOR  6
#define  NUM_FATS            2

/* sectors_per_disk is encoded as a signed int */
#define MAX_SECTORS_PER_DISK 0x7FFFFFFF
#define MAX_DISK_SIZE ((Wide)MAX_SECTORS_PER_DISK * BYTES_PER_SECTOR)

typedef long long      Wide;   /* might be something else if you don't use GCC */
typedef unsigned char  Byte;
typedef Byte*          Bytes;

#define  BYTE_(p,i)      (((Bytes)(p))[(i)])

#define  POKEB(p,v)     BYTE_(p,0) = (Byte)(v)
#define  POKES(p,v)   ( BYTE_(p,0) = (Byte)(v), BYTE_(p,1) = (Byte)((v) >> 8) )
#define  POKEW(p,v)   ( BYTE_(p,0) = (Byte)(v), BYTE_(p,1) = (Byte)((v) >> 8), BYTE_(p,2) = (Byte)((v) >> 16), BYTE_(p,3) = (Byte)((v) >> 24) )

static Byte  s_boot_sector   [ BYTES_PER_SECTOR ];   /* boot sector */
static Byte  s_fsinfo_sector [ BYTES_PER_SECTOR ];   /* FS Info sector */
static Byte  s_fat_head      [ BYTES_PER_SECTOR ];   /* first FAT sector */

// For handling Unicode paths
#ifdef _WIN32
    #define TCHAR       wchar_t
    #define ARGC        wargc
    #define ARGV        wargv
    #define STRINGPRINT "%S"
#else
    #define TCHAR       char
    #define ARGC        argc
    #define ARGV        argv
    #define STRINGPRINT "%s"
#endif

/* this is the date and time when creating the disk */
static int
get_serial_id( void )
{
    unsigned short  lo, hi;
    time_t          now = time(NULL);
    struct tm       tm  = gmtime( &now )[0];

    lo  = (unsigned short)(tm.tm_mday + ((tm.tm_mon+1) << 8) + (tm.tm_sec << 8));
    hi  = (unsigned short)(tm.tm_min + (tm.tm_hour << 8) + (tm.tm_year + 1900));

    return lo + (hi << 16);
}

static int
get_sectors_per_cluster( Wide  disk_size )
{
    Wide  disk_MB = disk_size/(1024*1024);

    if (disk_MB < 260)
        return 1;

    if (disk_MB < 8192)
        return 4;

    if (disk_MB < 16384)
        return 8;

    if (disk_MB < 32768)
        return 16;

    return 32;
}

static int
get_sectors_per_fat( Wide  disk_size, int  sectors_per_cluster )
{
    Wide   divider;

    /* weird computation from MS - see fatgen103.doc for details */
    disk_size -= RESERVED_SECTORS * BYTES_PER_SECTOR;  /* don't count 32 reserved sectors */
    disk_size /= BYTES_PER_SECTOR;       /* disk size in sectors */
    divider = ((256 * sectors_per_cluster) + NUM_FATS) / 2;

    return (int)( (disk_size + (divider-1)) / divider );
}

static void
boot_sector_init( Bytes  boot, Bytes  info, Wide   disk_size, const char*  label, int serial_id )
{
    int   sectors_per_cluster = get_sectors_per_cluster(disk_size);
    int   sectors_per_fat    = get_sectors_per_fat(disk_size, sectors_per_cluster);
    int   sectors_per_disk   = (int)(disk_size / BYTES_PER_SECTOR);
    int   free_count;
    serial_id == 0 ? get_serial_id(): serial_id;

    if (label == NULL)
        label = "SDCARD";

    POKEB(boot, 0xeb);
    POKEB(boot+1, 0x5a);
    POKEB(boot+2, 0x90);
    strcpy( (char*)boot + 3, "MSWIN4.1" );
    POKES( boot + 0x0b, BYTES_PER_SECTOR );    /* sector size */
    POKEB( boot + 0xd, sectors_per_cluster );  /* sectors per cluster */
    POKES( boot + 0xe, RESERVED_SECTORS );     /* reserved sectors before first FAT */
    POKEB( boot + 0x10, NUM_FATS );            /* number of FATs */
    POKES( boot + 0x11, 0 );                   /* max root directory entries for FAT12/FAT16, 0 for FAT32 */
    POKES( boot + 0x13, 0 );                   /* total sectors, 0 to use 32-bit value at offset 0x20 */
    POKEB( boot + 0x15, 0xF8 );                /* media descriptor, 0xF8 == hard disk */
    POKES( boot + 0x16, 0 );                   /* Sectors per FAT for FAT12/16, 0 for FAT32 */
    POKES( boot + 0x18, 9 );                   /* Sectors per track (whatever) */
    POKES( boot + 0x1a, 2 );                   /* Number of heads (whatever) */
    POKEW( boot + 0x1c, 0 );                   /* Hidden sectors */
    POKEW( boot + 0x20, sectors_per_disk );    /* Total sectors */

    /* extension */
    POKEW( boot + 0x24, sectors_per_fat );       /* Sectors per FAT */
    POKES( boot + 0x28, 0 );         /* FAT flags */
    POKES( boot + 0x2a, 0 );         /* version */
    POKEW( boot + 0x2c, 2 );         /* cluster number of root directory start */
    POKES( boot + 0x30, 1 );         /* sector number of FS information sector */
    POKES( boot + 0x32, BACKUP_BOOT_SECTOR );         /* sector number of a copy of this boot sector */
    POKEB( boot + 0x40, 0x80 );      /* physical drive number */
    POKEB( boot + 0x42, 0x29 );      /* extended boot signature ?? */
    POKEW( boot + 0x43, serial_id ); /* serial ID */
    strncpy( (char*)boot + 0x47, label, 11 );  /* Volume Label */
    memcpy( boot + 0x52, "FAT32   ", 8 );  /* FAT system type, padded with 0x20 */

    POKEB( boot + BYTES_PER_SECTOR-2, 0x55 );    /* boot sector signature */
    POKEB( boot + BYTES_PER_SECTOR-1, 0xAA );

    /* FSInfo sector */
    free_count = sectors_per_disk - 32 - 2*sectors_per_fat;

    POKEW( info + 0,   0x41615252 );
    POKEW( info + 484, 0x61417272 );
    POKEW( info + 488, free_count );   /* number of free clusters */
    POKEW( info + 492, 3 );            /* next free clusters, 0-1 reserved, 2 is used for the root dir */
    POKEW( info + 508, 0xAA550000 );
}

static void
fat_init( Bytes  fat )
{
    POKEW( fat,     0x0ffffff8 );  /* reserve cluster 1, media id in low byte */
    POKEW( fat + 4, 0x0fffffff );  /* reserve cluster 2 */
    POKEW( fat + 8, 0x0fffffff );  /* end of clust chain for root dir */
}


static int
write_sector( FILE*  file, Bytes  sector )
{
    int result = fwrite( sector, 1, BYTES_PER_SECTOR, file ) != BYTES_PER_SECTOR;
    if (result) {
      fprintf(stderr, "Failed to write sector of %d bytes: %s\n", BYTES_PER_SECTOR, strerror(errno));
    }
    return result;
}

static int
write_empty( FILE*   file, Wide  count )
{
    static  Byte  empty[256*1024];
    memset(empty, 0, sizeof(empty));

    count *= BYTES_PER_SECTOR;
    while (count > 0) {
        int  len = sizeof(empty);
        if (len > count)
          len = count;
        if ( fwrite( empty, 1, len, file ) != (size_t)len ) {
            fprintf(stderr, "Failed to write %d bytes: %s\n", len, strerror(errno));
            return 1;
        }

        count -= len;
    }
    return 0;
}

static void usage (void)
{
    fprintf(stderr, "mksdcard: create a blank FAT32 image to be used with the Android emulator\n" );
    fprintf(stderr, "usage: mksdcard [-l label] <size> <file>\n\n");
    fprintf(stderr, "  if <size> is a simple integer, it specifies a size in bytes\n" );
    fprintf(stderr, "  if <size> is an integer followed by 'K', it specifies a size in KiB\n" );
    fprintf(stderr, "  if <size> is an integer followed by 'M', it specifies a size in MiB\n" );
    fprintf(stderr, "  if <size> is an integer followed by 'G', it specifies a size in GiB\n" );
    fprintf(stderr, "\nMinimum size is 9M. The Android emulator cannot use smaller images.\n" );
    fprintf(stderr, "Maximum size is %lld bytes, %lldK, %lldM or %lldG\n",
            MAX_DISK_SIZE, MAX_DISK_SIZE >> 10, MAX_DISK_SIZE >> 20, MAX_DISK_SIZE >> 30);
    exit(1);
}

int  main( int argc, char**  argv )
{
    Wide          disk_size;
    int           sectors_per_fat;
    int           sectors_per_disk;
    TCHAR*        end;
    const TCHAR*  label = NULL;
    int           serial_id = 0;
    FILE*         f = NULL;

#ifdef _WIN32
    int      wargc;
    TCHAR**  wargv;
    TCHAR**  unused_environment;
    TCHAR**  unused_startupinfo;

#ifdef _MSC_VER
    wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
#else
    __wgetmainargs(&wargc, &wargv, &unused_environment, 0, &unused_startupinfo);
#endif
#endif

    for ( ; ARGC > 1 && ARGV[1][0] == '-'; ARGC--, ARGV++ )
    {
        TCHAR*  arg = ARGV[1] + 1;
        switch (arg[0]) {
            case 'l':
                if (arg[1] != 0)
                    arg += 2;
                else {
                    ARGC--;
                    ARGV++;
                    if (ARGC <= 1)
                        usage();
                    arg = ARGV[1];
                }
                label = arg;
                break;
            case 's':
                if (arg[1] != 0)
                    arg += 2;
                else {
                    ARGC--;
                    ARGV++;
                    if (ARGC <= 1)
                        usage();
                    arg = ARGV[1];
                }
                serial_id = atoi(arg);
                break;
            default:
                usage();
        }
    }

    if (ARGC != 3)
        usage();

#ifdef _WIN32
    disk_size = wcstoll( ARGV[1], &end, 10 );
#else
    disk_size = strtoll( ARGV[1], &end, 10 );
#endif

    if (disk_size <= 0 || errno == EINVAL || errno == ERANGE) {
        fprintf(stderr, "Invalid argument size '" STRINGPRINT "'\n\n", ARGV[1]);
        usage();
    }

    if (*end == 'K')
        disk_size *= 1024;
    else if (*end == 'M')
        disk_size *= 1024*1024;
    else if (*end == 'G')
        disk_size *= 1024*1024*1024;

    if (disk_size < 9*1024*1024) {
        fprintf(stderr, "Invalid argument: size '" STRINGPRINT "' is too small.\n\n", ARGV[1]);
        usage();
    } else if (disk_size > MAX_DISK_SIZE) {
        fprintf(stderr, "Invalid argument: size '" STRINGPRINT "' is too large.\n\n", ARGV[1]);
        usage();
    }

    sectors_per_disk = disk_size / BYTES_PER_SECTOR;
    sectors_per_fat  = get_sectors_per_fat( disk_size, get_sectors_per_cluster( disk_size ) );

    boot_sector_init( s_boot_sector, s_fsinfo_sector, disk_size, NULL, serial_id);
    fat_init( s_fat_head );

#ifdef _WIN32
    f = _wfopen( ARGV[2], L"wb" );
#else
    f = fopen( ARGV[2], "wb" );
#endif
    if ( !f ) {
      fprintf(stderr, "Could not create file '" STRINGPRINT "': %s\n", ARGV[2], strerror(errno));
      goto FailWrite;
    }

   /* here's the layout:
    *
    *  boot_sector
    *  fsinfo_sector
    *  empty
    *  backup boot sector
    *  backup fsinfo sector
    *  RESERVED_SECTORS - 4 empty sectors (if backup sectors), or RESERVED_SECTORS - 2 (if no backup)
    *  first fat
    *  second fat
    *  zero sectors
   */

    if ( write_sector( f, s_boot_sector ) ) goto FailWrite;
    if ( write_sector( f, s_fsinfo_sector ) ) goto FailWrite;
    if ( BACKUP_BOOT_SECTOR > 0 ) {
        if ( write_empty( f, BACKUP_BOOT_SECTOR - 2 ) ) goto FailWrite;
        if ( write_sector( f, s_boot_sector ) ) goto FailWrite;
        if ( write_sector( f, s_fsinfo_sector ) ) goto FailWrite;
        if ( write_empty( f, RESERVED_SECTORS - 2 - BACKUP_BOOT_SECTOR ) ) goto FailWrite;
    }
    else if ( write_empty( f, RESERVED_SECTORS - 2 ) ) goto FailWrite;

    if ( write_sector( f, s_fat_head ) ) goto FailWrite;
    if ( write_empty( f, sectors_per_fat-1 ) ) goto FailWrite;

    if ( write_sector( f, s_fat_head ) ) goto FailWrite;
    if ( write_empty( f, sectors_per_fat-1 ) ) goto FailWrite;

    if ( write_empty( f, sectors_per_disk - RESERVED_SECTORS - 2*sectors_per_fat ) ) goto FailWrite;

    fclose(f);
    return 0;

FailWrite:
    if (f != NULL) {
      fclose(f);
#ifdef _WIN32
      _wunlink( ARGV[2] );
#else
      unlink( ARGV[2] );
#endif
      fprintf(stderr, "File '" STRINGPRINT "' was not created.\n", ARGV[2]);
    }
    return 1;
}
