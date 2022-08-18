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
/* implement the modem character device for Android within the QEMU event loop.
 * it communicates through a serial port with "rild" (Radio Interface Layer Daemon)
 * on the emulated device.
 */
#include "android/telephony/modem_driver.h"

#include "android/telephony/debug.h"
#include "android/utils/debug.h"

#include <string.h>

#define  xxDEBUG

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ) )
#else
#  define  D(...)   ((void)0)
#endif

AModem        android_modem;
CSerialLine*  android_modem_serial_line;

typedef struct {
    CSerialLine*      serial_line;
    AModem            modem;
    char              in_buff[ 1024 ];
    int               in_pos;
    int               in_sms;
} ModemDriver;

/* send unsolicited messages to the device */
static void
modem_driver_unsol( void*  _md, const char*  message)
{
    ModemDriver*      md = _md;
    int               len = strlen(message);

    android_serialline_write(md->serial_line, (const uint8_t*)message, len);
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
            answer = amodem_send(android_modem, md->in_buff);
            if (answer != NULL) {
                D( "%s: >> %s\n", __FUNCTION__, answer );
                len = strlen(answer);
                if (len == 2 && answer[0] == '>' && answer[1] == ' ')
                    md->in_sms = 1;

                android_serialline_write(md->serial_line, (const uint8_t*)answer, len);
                android_serialline_write(md->serial_line, (const uint8_t*)"\r", 1);
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
modem_driver_init(int base_port, ModemDriver* dm, CSerialLine* sl, int sim_present) {
    dm->serial_line = sl;
    dm->in_pos = 0;
    dm->in_sms = 0;
    dm->modem = amodem_create( base_port, modem_driver_unsol, sim_present, dm );

    android_serialline_addhandlers(sl,
                                   dm,
                                   modem_driver_can_read,
                                   modem_driver_read);
}

static ModemDriver  modem_driver[1];

void android_modem_init( int  base_port, int sim_present )
{
    android_telephony_debug_modem = VERBOSE_CHECK(modem);
    android_telephony_debug_radio = VERBOSE_CHECK(radio);
    android_telephony_debug_socket = VERBOSE_CHECK(socket);

    if (android_modem_serial_line != NULL) {
        modem_driver_init(base_port, modem_driver, android_modem_serial_line, sim_present);
        android_modem = modem_driver->modem;
    }
}

void android_modem_driver_send_nitz_now() {
    ModemDriver* md = modem_driver;
    // It might be uninitialized in offworld unit tests
    if (!md->modem) {
        return;
    }
    const char*  answer = amodem_send_unsol_nitz(md->modem);
    android_serialline_write(md->serial_line, (const uint8_t*)answer, strlen(answer));
    android_serialline_write(md->serial_line, (const uint8_t*)"\r", 1);
}

AModem android_modem_get() {
    return android_modem;
}
