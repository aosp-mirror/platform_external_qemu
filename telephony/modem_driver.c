/* Copyright (C) 2007-2008, 2012 The Android Open Source Project
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
/* implement the modem character device for Android within the QEMU event loop.
 * it communicates through a serial port with "rild" (Radio Interface Layer Daemon)
 * on the emulated device.
 */
#include "modem_driver.h"
#include "sysemu/char.h"
#include "android/utils/debug.h"

#define  xxDEBUG

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ) )
#else
#  define  D(...)   ((void)0)
#endif

#define  D(...)    VERBOSE_PRINT(modem,__VA_ARGS__)

AModem            android_modem[2];
CharDriverState*  android_modem_cs[2];

typedef struct {
    CharDriverState*  cs;
    AModem            modem;
    char              in_buff[ 1024 ];
    int               in_pos;
    int               in_sms;
} ModemDriver;

/* send unsollicited messages to the device */
static void
modem_driver_unsol( void*  _md, const char*  message)
{
    ModemDriver*      md = _md;
    int               len = strlen(message);

    D( "%s: Modem#0 sends data\n", __FUNCTION__);
    qemu_chr_write(md->cs, (const uint8_t*)message, len);
}

/* send unsollicited messages to the device */
static void
modem1_driver_unsol( void*  _md, const char*  message)
{
    ModemDriver*      md = _md;
    int               len = strlen(message);

    D( "%s: Modem#1 sends data\n", __FUNCTION__);
    qemu_chr_write(md->cs, (const uint8_t*)message, len);
}

static int
modem_driver_can_read( void*  _md )
{
    ModemDriver*  md  = _md;
    int           ret = sizeof(md->in_buff) - md->in_pos;

    return ret;
}

/* despite its name, this function is called when the device writes to the modem */
static void
modem_driver_read( void*  _md, const uint8_t*  src, int  len )
{
    ModemDriver*      md  = _md;
    const uint8_t*    end = src + len;
    int               nn;

    D( "%s: reading %d from %p bytes:", __FUNCTION__, len, src );
    for (nn = 0; nn < len; nn++) {
        int  c = src[nn];
        if (c >= 32 && c < 127)
            D( "%c", c );
        else if (c == '\n')
            D( "<LF>" );
        else if (c == '\r')
            D( "<CR>" );
        else
            D( "\\x%02x", c );
    }
    D( "\n" );

    for ( ; src < end; src++ ) {
        char  c = src[0];

        if (md->in_sms) {
            if (c != 26)
                goto AppendChar;

            md->in_buff[ md->in_pos ] = c;
            md->in_pos++;
            md->in_sms = 0;
            c = '\n';
        }

        if (c == '\n' || c == '\r') {
            const char*  answer;

            if (md->in_pos == 0)  /* skip empty lines */
                continue;

            md->in_buff[ md->in_pos ] = 0;
            md->in_pos                = 0;

            D( "%s: << %s\n", __FUNCTION__, md->in_buff );
            answer = amodem_send(android_modem[0], md->in_buff);
            if (answer != NULL) {
                D( "%s: >> %s\n", __FUNCTION__, answer );
                len = strlen(answer);
                if (len == 2 && answer[0] == '>' && answer[1] == ' ')
                    md->in_sms = 1;

                qemu_chr_write(md->cs, (const uint8_t*)answer, len);
                qemu_chr_write(md->cs, (const uint8_t*)"\r", 1);
            } else
                D( "%s: -- NO ANSWER\n", __FUNCTION__ );

            continue;
        }
    AppendChar:
        md->in_buff[ md->in_pos++ ] = c;
        if (md->in_pos == sizeof(md->in_buff)) {
            /* input is too long !! */
            md->in_pos = 0;
        }
    }
    D( "%s: done\n", __FUNCTION__ );
}

/* despite its name, this function is called when the device writes to the modem */
static void
modem1_driver_read( void*  _md, const uint8_t*  src, int  len )
{
    ModemDriver*      md  = _md;
    const uint8_t*    end = src + len;
    int               nn;

    D( "\n%s: Modem#1 received data", __FUNCTION__);
    D( "\n%s: reading %d from %p bytes:", __FUNCTION__, len, src );
    for (nn = 0; nn < len; nn++) {
        int  c = src[nn];
        if (c >= 32 && c < 127)
            D( "%c", c );
        else if (c == '\n')
            D( "<LF>" );
        else if (c == '\r')
            D( "<CR>" );
        else
            D( "\\x%02x", c );
    }
    D( "\n" );

    for ( ; src < end; src++ ) {
        char  c = src[0];

        if (md->in_sms) {
            if (c != 26)
                goto AppendChar;

            md->in_buff[ md->in_pos ] = c;
            md->in_pos++;
            md->in_sms = 0;
            c = '\n';
        }

        if (c == '\n' || c == '\r') {
            const char*  answer;

            if (md->in_pos == 0)  /* skip empty lines */
                continue;

            md->in_buff[ md->in_pos ] = 0;
            md->in_pos                = 0;

            D( "%s: << %s\n", __FUNCTION__, md->in_buff );
            answer = amodem_send(android_modem[1], md->in_buff);
            if (answer != NULL) {
                D( "%s: >> %s\n", __FUNCTION__, answer );
                len = strlen(answer);
                if (len == 2 && answer[0] == '>' && answer[1] == ' ')
                    md->in_sms = 1;

                qemu_chr_write(md->cs, (const uint8_t*)answer, len);
                qemu_chr_write(md->cs, (const uint8_t*)"\r", 1);
            } else
                D( "%s: -- NO ANSWER\n", __FUNCTION__ );

            continue;
        }
    AppendChar:
        md->in_buff[ md->in_pos++ ] = c;
        if (md->in_pos == sizeof(md->in_buff)) {
            /* input is too long !! */
            md->in_pos = 0;
        }
    }
    D( "%s: done\n", __FUNCTION__ );
}


static void
modem_driver_init( int  base_port, ModemDriver*  dm, CharDriverState*  cs , AModemUnsolFunc  unsol_func, IOReadHandler read_func, int index)
{
    dm->cs     = cs;
    dm->in_pos = 0;
    dm->in_sms = 0;
    D( "\n%s:Creating the Modem ", __FUNCTION__ );
    dm->modem  = amodem_create( base_port, unsol_func, dm, index );

    D( "\n%s:Adding Modem driver read handlers for this modem ", __FUNCTION__ );
    qemu_chr_add_handlers( cs, modem_driver_can_read, read_func, NULL, dm );
}


void android_modem_init( int  base_port )
{
    static ModemDriver  modem_driver[2];

    D( "%s: \n", __FUNCTION__ );
    if (android_modem_cs[0] != NULL ||
        android_modem_cs[1] != NULL) {
        D( "\n%s: Char device(qemud) b/w rild and modem is already up", __FUNCTION__ );
        D( "\n%s: Calling modem init for modem0", __FUNCTION__ );
        modem_driver_init( base_port, &modem_driver[0], android_modem_cs[0], modem_driver_unsol, modem_driver_read, 0);
        android_modem[0] = modem_driver[0].modem;
        D( "\n%s: Calling modem init for modem1", __FUNCTION__ );
        modem_driver_init( base_port, &modem_driver[1], android_modem_cs[1], modem1_driver_unsol, modem1_driver_read, 1);
        android_modem[1] = modem_driver[1].modem;
    }
}
