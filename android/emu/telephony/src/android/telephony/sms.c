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
#include "android/telephony/sms.h"

#include "android/telephony/debug.h"
#include "android/telephony/gsm.h"
#include "android/utils/timezone.h"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Debug logs.
#define  D(...)  ANDROID_TELEPHONY_LOG_ON(modem,__VA_ARGS__)

/* maximum number of payload bytes in a message */
#define  MAX_USER_DATA_BYTES   140

/* maximum number of payload septets in a message */
#define  MAX_USER_DATA_SEPTETS  160

/* maximum number of payload bytes in a message if a user data header is present */
#define  MAX_USER_DATA_BYTES_WITH_HEADER   134

/* maximum number of payload septets in a message if a user data header is present*/
#define  MAX_USER_DATA_SEPTETS_WITH_HEADER   153

/* ETSI TS 123 040 9.1.2.5 Address fields */
#define TON_DOMESTIC      0
#define TON_INTERNATIONAL 1
#define TON_ALPHANUMERIC  5

static int
sms_build_toa( int ton )
{
    return 0x81 | (ton << 4);
}

static int
sms_get_ton( int toa )
{
    return (toa >> 4) & 7;
}

/** MESSAGE TEXT
 **/
int
sms_utf8_from_message_str( const char*  str, int  strlen, unsigned char*  utf8, int  utf8len )
{
    cbytes_t  p       = (cbytes_t)str;
    cbytes_t  end     = p + strlen;
    int       count   = 0;
    int       escaped = 0;

    while (p < end)
    {
        int  c = p[0];

        /* read the value from the string */
        p += 1;
        if (c >= 128) {
            if ((c & 0xe0) == 0xc0)
                c &= 0x1f;
            else if ((c & 0xf0) == 0xe0)
                c &= 0x0f;
            else
                c &= 0x07;

            while (p < end && (p[0] & 0xc0) == 0x80) {
                c = (c << 6) | (p[0] & 0x3f);
                p++;
            }
        }
        if (escaped) {
            switch (c) {
                case '\\':
                    break;
                case 'n':  /* \n is line feed */
                    c = 10;
                    break;

                case 'x':  /* \xNN, where NN is a 2-digit hexadecimal value */
                    if (p+2 > end)
                        return -1;
                    c = gsm_hex2_to_byte( (const char*)p );
                    if (c < 0)
                        return -1;
                    p += 2;
                    break;

                case 'u':  /* \uNNNN where NNNN is a 4-digiti hexadecimal value */
                    if (p + 4 > end)
                        return -1;
                    c = gsm_hex4_to_short( (const char*)p );
                    if (c < 0)
                        return -1;
                    p += 4;
                    break;

                default:  /* invalid escape, return -1 */
                    return -1;
            }
            escaped = 0;
        }
        else if (c == '\\')
        {
            escaped = 1;
            continue;
        }

        /* now, try to write it to the destination */
        if (c < 128) {
            if (count < utf8len)
                utf8[count] = (byte_t) c;
            count += 1;
        }
        else if (c < 0x800) {
            if (count < utf8len)
                utf8[count]   = (byte_t)(0xc0 | ((c >> 6) & 0x1f));
            if (count+1 < utf8len)
                utf8[count+1] = (byte_t)(0x80 | (c & 0x3f));
            count += 2;
        }
        else {
            if (count < utf8len)
                utf8[count]   = (byte_t)(0xc0 | ((c >> 12) & 0xf));
            if (count+1 < utf8len)
                utf8[count+1] = (byte_t)(0x80 | ((c >> 6) & 0x3f));
            if (count+2 < utf8len)
                utf8[count+2] = (byte_t)(0x80 | (c & 0x3f));
            count += 3;
        }
    }

    if (escaped)   /* bad final escape */
        return -1;

    return count;
}

/* to convert utf-8 to a message string, we only need to deal with control characters
 * and that's it */
int  sms_utf8_to_message_str( const unsigned char*  utf8, int  utf8len, char*  str, int  strlen )
{
    cbytes_t  p = utf8;
    cbytes_t  end = p + utf8len;
    int       count   = 0;

    while (p < end)
    {
        int  c      = p[0];
        int  escape = 0;

        /* read the value from the string */
        p += 1;
        if (c >= 128) {
            if ((c & 0xe0) == 0xc0)
                c &= 0x1f;
            else if ((c & 0xf0) == 0xe0)
                c &= 0x0f;
            else
                c &= 0x07;
            p++;
            while (p < end && (p[0] & 0xc0) == 0x80) {
                c = (c << 6) | (p[0] & 0x3f);
                p++;
            }
        }

        if (c < ' ') {
            escape = 1;
            if (c == '\n') {
                c      = 'n';
                escape = 2;
            }
        }
        else if (c == '\\')
            escape = 2;

        switch (escape) {
            case 0:
                if (c < 128) {
                    if (count < strlen)
                        str[count] = (char) c;
                    count += 1;
                }
                else if (c < 0x800) {
                    if (count < strlen)
                        str[count]   = (byte_t)(0xc0 | ((c >> 6) & 0x1f));
                    if (count+1 < strlen)
                        str[count+1] = (byte_t)(0x80 | (c & 0x3f));
                    count += 2;
                }
                else {
                    if (count < strlen)
                        str[count]   = (byte_t)(0xc0 | ((c >> 12) & 0xf));
                    if (count+1 < strlen)
                        str[count+1] = (byte_t)(0x80 | ((c >> 6) & 0x3f));
                    if (count+2 < strlen)
                        str[count+2] = (byte_t)(0x80 | (c & 0x3f));
                    count += 3;
                }
                break;

            case 1:
                if (count+3 < strlen) {
                    str[count+0] = '\\';
                    str[count+1] = 'x';
                    gsm_hex_from_byte(str + count + 2, c);
                }
                count += 4;
                break;

            default:
                if (count+2 < strlen) {
                    str[count+0] = '\\';
                    str[count+1] = (char) c;
                }
                count += 2;
        }
    }
    return count;
}


/** TIMESTAMPS
 **/
void
sms_timestamp_now( SmsTimeStamp  stamp )
{
    time_t     now_time = time(NULL);
    struct tm  local    = *localtime(&now_time);
    int        tzdiff   = android_tzoffset_in_seconds(&now_time) / (15 * 60); /*tzdiff is in number of quater-hours*/

    stamp->data[0] = gsm_int_to_bcdi( local.tm_year % 100 );
    stamp->data[1] = gsm_int_to_bcdi( local.tm_mon+1 );
    stamp->data[2] = gsm_int_to_bcdi( local.tm_mday );
    stamp->data[3] = gsm_int_to_bcdi( local.tm_hour );
    stamp->data[4] = gsm_int_to_bcdi( local.tm_min );
    stamp->data[5] = gsm_int_to_bcdi( local.tm_sec );


    stamp->data[6] = gsm_int_to_bcdi( tzdiff >= 0 ? tzdiff : -tzdiff );
    if (tzdiff < 0)
        stamp->data[6] |= 0x08;
}

int
sms_timestamp_to_tm( SmsTimeStamp  stamp, struct tm*  tm )
{
    int  tzdiff;

    tm->tm_year = gsm_int_from_bcdi( stamp->data[0] );
    if (tm->tm_year < 50)
        tm->tm_year += 100;
    tm->tm_mon  = gsm_int_from_bcdi( stamp->data[1] ) -1;
    tm->tm_mday = gsm_int_from_bcdi( stamp->data[2] );
    tm->tm_hour = gsm_int_from_bcdi( stamp->data[3] );
    tm->tm_min  = gsm_int_from_bcdi( stamp->data[4] );
    tm->tm_sec  = gsm_int_from_bcdi( stamp->data[5] );

    tm->tm_isdst = -1;

    tzdiff = gsm_int_from_bcdi( stamp->data[6] & 0xf7 );
    if (stamp->data[6] & 0x8)
        tzdiff = -tzdiff;

    return tzdiff;
}

static void
gsm_rope_add_timestamp( GsmRope  rope, const SmsTimeStampRec*  ts )
{
    gsm_rope_add( rope, ts->data, 7 );
}


/** SMS ADDRESSES
 **/

/* 3G TS 23.038 6.2.1 Default alphabet (the ASCII part) */
int
is_in_gsm_default_alphabet( int c ) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return 1;
    } else {
        switch (c) {
        case '@':
        case '_':
        case ' ':
        case '!':
        case '"':
        case '#':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case '-':
        case '.':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
            return 1;

        default:
            return 0;
        }
    }
}

/* 3G TS 23.038 6.1.2 Character packing */
static int
sms_alphanum_address_from_str( SmsAddress  address, const char*  src, int  srclen )
{
    if (srclen == 0) {
        return -1;
    } else if (srclen > MAX_ALPHANUMERIC_SMS_ADDRESS_LENGTH) {
        return -1;
    } else {
        const char* end = src + srclen;
        bytes_t data = address->data;
        int shifterBits = 0;
        int shifterSize = 0;

        memset( data, 0, sizeof(address->data) );

        /* src --(7bit)-->  shifterBits --(8bit)--> data */
        for (; src != end; ++src) {
            const char c = *src;
            if (is_in_gsm_default_alphabet(c)) {
                shifterBits |= (c << shifterSize);
                shifterSize += BITS_PER_SMS_CHAR;

                while (shifterSize >= CHAR_BIT) {
                    *data = shifterBits & 0xff;
                    ++data;

                    shifterBits >>= CHAR_BIT;
                    shifterSize -= CHAR_BIT;
                }
            } else {
                return -1;
            }
        }

        if (shifterSize > 0) {
            *data = shifterBits;
        }

        address->len = (srclen * BITS_PER_SMS_CHAR + BITS_PER_SEMIOCTET - 1) /
            BITS_PER_SEMIOCTET;
        address->toa = sms_build_toa(TON_ALPHANUMERIC);

        return 0;
    }
}

static int
sms_numeric_address_from_str( SmsAddress  address, const char*  src, int  srclen )
{
    const char*  end   = src + srclen;
    int          shift = 0, len = 0;
    bytes_t      data = address->data;

    address->len = 0;
    address->toa = sms_build_toa(TON_DOMESTIC);

    if (src >= end)
        return -1;

    if ( src[0] == '+' ) {
        address->toa = sms_build_toa(TON_INTERNATIONAL);
        if (++src == end)
            goto Fail;
    }

    memset( address->data, 0, sizeof(address->data) );

    shift = 0;

    while (src < end) {
        int  c = *src++;
        if (c >= '0' && c <= '9') {
            if (data >= address->data + sizeof(address->data) )
                goto Fail;

            c -= '0';
            data[0] |= c << shift;
            len   += 1;
            shift += 4;
            if (shift == 8) {
                shift = 0;
                data += 1;
            }
        } // endif (0..9)
    }
    if (shift != 0)
        data[0] |= 0xf0;

    address->len = len;
    return (len > 0) ? 0 : -1;

Fail:
    return -1;
}

int
sms_address_from_str( SmsAddress  address, const char*  src, int  srclen )
{
    int result;
    result = sms_numeric_address_from_str(address, src, srclen);
    if (result >= 0) {
        return result;
   }

    return sms_alphanum_address_from_str(address, src, srclen);
}

static int
sms_numeric_address_to_str( SmsAddress address, char*  str, int  strlen )
{
    static const char  dialdigits[16] = "0123456789*#,N%";
    int                n, count = 0;

    if (address->toa == 0x91) {
        if (count < strlen)
            str[count] = '+';
        count++;
    }
    for (n = 0; n < address->len; n += 2)
    {
        int   c = address->data[n/2];

        if (count < strlen)
            str[count] = dialdigits[c & 0xf];
        count += 1;

        if (n+1 > address->len)
            break;

        if (count < strlen)
            str[count] = dialdigits[(c >> 4) & 0xf];
        if (str[count])
            count += 1;
    }
    return count;
}

static int
sms_alphanumeric_address_to_str( SmsAddress address, char*  str, int  strlen )
{
    int shifterBits = 0;
    int shifterSize = 0;
    int length = 0;
    int addressLengthInBits = address->len * BITS_PER_SEMIOCTET;

    cbytes_t i = address->data;
    cbytes_t end = i + ((addressLengthInBits + CHAR_BIT - 1) / CHAR_BIT);

    /* address->data --(8 bits)--> shifterBits --(7 bit)--> str */
    for (; i != end; ++i) {
        int thisBits = (addressLengthInBits < CHAR_BIT) ?
            addressLengthInBits : CHAR_BIT;
        shifterBits |= (*i << shifterSize);
        shifterSize += thisBits;
        addressLengthInBits -= thisBits;

        while (shifterSize >= BITS_PER_SMS_CHAR) {
            char c = shifterBits & 0x7f;
            if (is_in_gsm_default_alphabet(c)) {
                 ++length;
                 if (length > strlen) {
                     return -1;
                 }

                *str = c;
                 ++str;

                 shifterBits >>= BITS_PER_SMS_CHAR;
                 shifterSize -= BITS_PER_SMS_CHAR;
            } else {
                return -1;
            }
        }
    }

    return length;
}

int
sms_address_to_str( SmsAddress address, char*  str, int  strlen )
{
    switch (sms_get_ton(address->toa)) {
    case TON_DOMESTIC:
    case TON_INTERNATIONAL:
        return sms_numeric_address_to_str(address, str, strlen);

    case TON_ALPHANUMERIC:
        return sms_alphanumeric_address_to_str(address, str, strlen);
    }

    return -1;
}

static void
gsm_rope_add_address( GsmRope  rope, const SmsAddressRec*  addr )
{
    gsm_rope_add_c( rope, addr->len );
    gsm_rope_add_c( rope, addr->toa );
    gsm_rope_add( rope, addr->data, (addr->len+1)/2 );
    if (addr->len & 1) {
        if (!rope->error && rope->data != NULL)
            rope->data[ rope->pos-1 ] |= 0xf0;
    }
}

static int
sms_address_eq( const SmsAddressRec*  addr1, const SmsAddressRec*  addr2 )
{
    if ( addr1->toa != addr2->toa ||
         addr1->len != addr2->len )
        return 0;

    return ( !memcmp( addr1->data, addr2->data, (addr1->len + 1) / 2 ));
}

/** SMS PARSER
 **/
static int
sms_get_byte( cbytes_t  *pcur, cbytes_t  end )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;

    if (cur < end) {
        result = cur[0];
        *pcur  = cur + 1;
    }
    return result;
}

/* parse a service center address, returns -1 in case of error */
static int
sms_get_sc_address( cbytes_t   *pcur,
                    cbytes_t    end,
                    SmsAddress  address )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;

    if (cur < end) {
        int  len = cur[0];
        int  dlen, adjust = 0;

        cur += 1;

        if (len == 0) {   /* empty address */
            address->len = 0;
            address->toa = 0x00;
            result       = 0;
            goto Exit;
        }

        if (cur + len > end) {
            goto Exit;
        }

        address->toa = *cur++;
        len         -= 1;
        result       = 0;

        for (dlen = 0; dlen < len; dlen+=1)
        {
            int  c = cur[dlen];
            int  v;

            adjust = 0;
            if (dlen >= sizeof(address->data)) {
                result = -1;
                break;
            }

            v = (c & 0xf);
            if (v >= 0xe)
                break;

            adjust              = 1;
            address->data[dlen] = (byte_t) c;

            v = (c >> 4) & 0xf;
            if (v >= 0xe) {
                break;
            }
        }
        address->len = 2*dlen + adjust;
    }
Exit:
    if (!result)
        *pcur = cur;

    return result;
}

static int
sms_skip_sc_address( cbytes_t   *pcur,
                     cbytes_t    end )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;
    int       len;

    if (cur >= end)
        goto Exit;

    len  = cur[0];
    cur += 1 + len;
    if (cur > end)
        goto Exit;

    *pcur  = cur;
    result = 0;
Exit:
    return result;
}

/* parse a sender/receiver address, returns -1 in case of error */
static int
sms_get_address( cbytes_t   *pcur,
                 cbytes_t    end,
                 SmsAddress  address )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;
    int       len, dlen;

    if (cur >= end)
        goto Exit;

    dlen = *cur++;

    if (dlen == 0) {
        address->len = 0;
        address->toa = 0;
        result       = 0;
        goto Exit;
    }

    if (cur + 1 + (dlen+1)/2 > end)
        goto Exit;

    address->len = dlen;
    address->toa = *cur++;

    len = (dlen + 1)/2;
    if (len > sizeof(address->data))
        goto Exit;

    memcpy( address->data, cur, len );
    cur   += len;
    result = 0;

Exit:
    if (!result)
        *pcur = cur;

    return result;
}

static int
sms_skip_address( cbytes_t   *pcur,
                  cbytes_t    end  )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;
    int       dlen;

    if (cur + 2 > end)
        goto Exit;

    dlen = cur[0];
    cur += 2 + (dlen + 1)/2;
    if (cur > end)
        goto Exit;

    *pcur = cur;
    result = 0;
Exit:
    return result;
}

/* parse a service center timestamp */
static int
sms_get_timestamp( cbytes_t     *pcur,
                   cbytes_t      end,
                   SmsTimeStamp  ts )
{
    cbytes_t  cur = *pcur;

    if (cur + 7 > end)
        return -1;

    memcpy( ts->data, cur, 7 );
    *pcur = cur + 7;
    return 0;
}

static int
sms_skip_timestamp( cbytes_t  *pcur,
                    cbytes_t   end )
{
    cbytes_t  cur = *pcur;

    if (cur + 7 > end)
        return -1;

    *pcur = cur + 7;
    return 0;
}


static int
sms_skip_validity_period( cbytes_t  *pcur,
                          cbytes_t   end,
                          int        mtiByte )
{
    cbytes_t  cur = *pcur;

    switch ((mtiByte >> 3) & 3) {
        case 1:  /* relative format */
            cur += 1;
            break;

        case 2:  /* enhanced format */
        case 3:  /* absolute format */
            cur += 7;
    }
    if (cur > end)
        return -1;

    *pcur = cur;
    return 0;
}

/** SMS PDU
 **/

typedef struct SmsPDURec {
    bytes_t  base;
    bytes_t  end;
    bytes_t  tpdu;
} SmsPDURec;

void
smspdu_free( SmsPDU  pdu )
{
    if (pdu) {
        free( pdu->base );
        pdu->base = NULL;
        pdu->end  = NULL;
        pdu->tpdu = NULL;
    }
}

SmsPduType
smspdu_get_type( SmsPDU  pdu )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte(&data, end);

    switch (mtiByte & 3) {
        case 0:  return SMS_PDU_DELIVER;
        case 1:  return SMS_PDU_SUBMIT;
        case 2:  return SMS_PDU_STATUS_REPORT;
        default: return SMS_PDU_INVALID;
    }
}

int
smspdu_get_sender_address( SmsPDU  pdu, SmsAddress  address )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte(&data, end);

    switch (mtiByte & 3) {
        case 0: /* SMS_PDU_DELIVER; */
            return sms_get_sc_address( &data, end, address );

        default: return -1;
    }
}

int
smspdu_get_sc_timestamp( SmsPDU  pdu, SmsTimeStamp  ts )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte( &data, end );

    switch (mtiByte & 3) {
        case 0:  /* SMS_PDU_DELIVER */
            {
                SmsAddressRec  address;

                if ( sms_get_sc_address( &data, end, &address ) < 0 )
                    return -1;

                data += 2;  /* skip protocol identifer + coding scheme */

                return sms_get_timestamp( &data, end, ts );
            }

        default: return -1;
    }
}

int
smspdu_get_receiver_address( SmsPDU  pdu, SmsAddress  address )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte( &data, end );

    switch (mtiByte & 3) {
        case 1:  /* SMS_PDU_SUBMIT */
            {
                data += 1;  /* skip message reference */
                return sms_get_address( &data, end, address );
            }

        default: return -1;
    }
}

typedef enum {
    SMS_CODING_SCHEME_UNKNOWN = 0,
    SMS_CODING_SCHEME_GSM7,
    SMS_CODING_SCHEME_UCS2

} SmsCodingScheme;

/* See ETSI TS 123 038 v14.0.0 (2017-04)
 *     3GPP TS 23.038 v14.0.0 Release 14
 *     section 4, SMS Data Coding Scheme.
 */
static SmsCodingScheme
sms_get_coding_scheme( cbytes_t  *pcur,
                       cbytes_t   end )
{
    cbytes_t  cur = *pcur;
    int       dataCoding;

    if (cur >= end)
        return SMS_CODING_SCHEME_UNKNOWN;

    dataCoding = *cur++;
    *pcur      = cur;

    if ((dataCoding & 0xC0) == 0x00) {
        if ((dataCoding & 0x0C) == 0x00) return SMS_CODING_SCHEME_GSM7;
        if ((dataCoding & 0x0C) == 0x08) return SMS_CODING_SCHEME_UCS2;
        return SMS_CODING_SCHEME_UNKNOWN; // 8-bit data or reserved
    }

    if ((dataCoding & 0xF0) == 0xF0) {
        if ((dataCoding & 0x04) == 0x00) return SMS_CODING_SCHEME_GSM7;
        return SMS_CODING_SCHEME_UNKNOWN; // 8-bit data
    }
    return SMS_CODING_SCHEME_UNKNOWN;
}


/* see TS 23.040 section 9.2.3.24 for details */
static int
sms_get_text_utf8( cbytes_t        *pcur,
                   cbytes_t         end,
                   int              hasUDH,
                   SmsCodingScheme  coding,
                   GsmRope          rope )
{
    cbytes_t  cur    = *pcur;
    int       result = -1;
    int       len;

    if (cur >= end)
        goto Exit;

    len = *cur++;

    /* skip user data header if any */
    int pad = 0;
    if ( hasUDH )
    {
        int  hlen;

        if (cur >= end)
            goto Exit;

        hlen = *cur++;
        if (cur + hlen > end)
            goto Exit;

        cur += hlen;

        if (coding == SMS_CODING_SCHEME_GSM7)
        {
            int  headerBits    = (hlen + 1) * CHAR_BIT;
            int  headerSeptets = headerBits / 7;
            if (headerBits % 7 > 0)
                headerSeptets += 1;
            len -= headerSeptets;
            pad = headerSeptets*7 - headerBits;
        }
        else
            len -= hlen+1;

        if (len < 0)
            goto Exit;
    }

    /* switch the user data header if any */
    if (coding == SMS_CODING_SCHEME_GSM7)
    {
        int  count = utf8_from_gsm7( cur, pad, len, NULL );

        if (rope != NULL)
        {
            bytes_t  dst = gsm_rope_reserve( rope, count );
            if (dst != NULL)
                utf8_from_gsm7( cur, pad, len, dst );
        }
    }
    else if (coding == SMS_CODING_SCHEME_UCS2)
    {
        int  count = ucs2_to_utf8( cur, len/2, NULL );

        if (rope != NULL)
        {
            bytes_t  dst = gsm_rope_reserve( rope, count );
            if (dst != NULL)
                ucs2_to_utf8( cur, len/2, dst );
        }
    }
    result = 0;

Exit:
    return result;
}

/* get the message embedded in a SMS PDU as a utf8 byte array, returns the length of the message in bytes */
/* or -1 in case of error */
int
smspdu_get_text_message( SmsPDU  pdu, unsigned char*  utf8, int  utf8len )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte( &data, end );

    switch (mtiByte & 3) {
        case 0:  /* SMS_PDU_DELIVER */
            {
                SmsAddressRec    address;
                SmsTimeStampRec  timestamp;
                SmsCodingScheme  coding;
                GsmRopeRec       rope[1];
                int              result;

                if ( sms_get_sc_address( &data, end, &address ) < 0 )
                    goto Fail;

                data  += 1;  /* skip protocol identifier */
                coding = sms_get_coding_scheme( &data, end );
                if (coding == SMS_CODING_SCHEME_UNKNOWN)
                    goto Fail;

                if ( sms_get_timestamp( &data, end, &timestamp ) < 0 )
                    goto Fail;

                if ( sms_get_text_utf8( &data, end, (mtiByte & 0x40), coding, rope ) < 0 )
                    goto Fail;

                result = rope->pos;
                if (utf8len > result)
                    utf8len = result;

                if (utf8len > 0)
                    memcpy( utf8, rope->data, utf8len );

                gsm_rope_done( rope );
                return result;
            }

        case 1:  /* SMS_PDU_SUBMIT */
            {
                SmsAddressRec    address;
                SmsCodingScheme  coding;
                GsmRopeRec       rope[1];
                int              result;

                data += 1;  /* message reference */

                if ( sms_get_address( &data, end, &address ) < 0 )
                    goto Fail;

                data  += 1;  /* skip protocol identifier */
                coding = sms_get_coding_scheme( &data, end );
                if (coding == SMS_CODING_SCHEME_UNKNOWN)
                    goto Fail;

                if ( sms_skip_validity_period( &data, end, mtiByte ) < 0 )
                    goto Fail;
                gsm_rope_init_alloc( rope, 0 );
                if ( sms_get_text_utf8( &data, end, (mtiByte & 0x40), coding, rope ) < 0 ) {
                    gsm_rope_done( rope );
                    goto Fail;
                }

                result = rope->pos;
                if (utf8len > result)
                    utf8len = result;

                if (utf8len > 0)
                    memcpy( utf8, rope->data, utf8len );

                gsm_rope_done( rope );
                return result;
            }
    }
Fail:
    return -1;
}

static cbytes_t
smspdu_get_user_data_ref( SmsPDU  pdu )
{
    cbytes_t  data    = pdu->tpdu;
    cbytes_t  end     = pdu->end;
    int       mtiByte = sms_get_byte( &data, end );
    int       len;

    /* if there is no user-data-header, there is no message reference here */
    if ((mtiByte & 0x40) == 0)
        goto Fail;

    switch (mtiByte & 3) {
        case 0:  /* SMS_PDU_DELIVER */
            if ( sms_skip_address( &data, end ) < 0 )
                goto Fail;

            data  += 2;  /* skip protocol identifier + coding scheme */

            if ( sms_skip_timestamp( &data, end ) < 0 )
                goto Fail;

            break;

        case 1:  /* SMS_PDU_SUBMIT */
            data += 1;  /* skip message reference */

            if ( sms_skip_address( &data, end ) < 0 )
                goto Fail;

            data += 2;  /* skip protocol identifier + coding scheme */
            if ( sms_skip_validity_period( &data, end, mtiByte ) < 0 )
                goto Fail;

            break;

        default:
            goto Fail;
    }

    /* skip user-data length */
    if (data+1 >= end)
        goto Fail;

    len   = data[1];
    data += 2;

    while (len >= 2 && data + 2 <= end) {
        int  htype = data[0];
        int  hlen = data[1];

        if (htype == 00 && hlen == 3 && data + 5 <= end) {
            return data + 2;
        }

        data += hlen;
        len  -= hlen - 2;
    }
Fail:
    return NULL;
}

int
smspdu_get_ref( SmsPDU  pdu )
{
    cbytes_t  user_ref = smspdu_get_user_data_ref( pdu );

    if (user_ref != NULL)
    {
        return user_ref[0];
    }
    else
    {
        cbytes_t  data    = pdu->tpdu;
        cbytes_t  end     = pdu->end;
        int       mtiByte = sms_get_byte( &data, end );

        if ((mtiByte & 3) == 1) {
            /* try to extract directly the reference for a SMS-SUBMIT */
            if (data < end)
                return data[0];
        }
    }
    return -1;
}

int
smspdu_get_max_index( SmsPDU  pdu )
{
    cbytes_t  user_ref = smspdu_get_user_data_ref( pdu );

    if (user_ref != NULL) {
        return user_ref[1];
    } else {
        return 1;
    }
}

int
smspdu_get_cur_index( SmsPDU  pdu )
{
    cbytes_t  user_ref = smspdu_get_user_data_ref( pdu );

    if (user_ref != NULL) {
        return user_ref[2] - 1;
    } else {
        return 0;
    }
}


static void
gsm_rope_add_sms_user_header( GsmRope  rope,
                              int      ref_number,
                              int      pdu_count,
                              int      pdu_index )
{
    gsm_rope_add_c( rope, 0x05 );     /* total header length == 5 bytes */
    gsm_rope_add_c( rope, 0x00 );     /* element id: concatenated message reference number */
    gsm_rope_add_c( rope, 0x03 );     /* element len: 3 bytes */
    gsm_rope_add_c( rope, (byte_t)ref_number );  /* reference number */
    gsm_rope_add_c( rope, (byte_t)pdu_count );     /* max pdu index */
    gsm_rope_add_c( rope, (byte_t)pdu_index+1 );   /* current pdu index */
}

/* write a SMS-DELIVER PDU into a rope */
static void
gsm_rope_add_sms_deliver_pdu( GsmRope                 rope,
                              cbytes_t                utf8,
                              int                     utf8len,
                              int                     use_gsm7,
                              const SmsAddressRec*    sender_address,
                              const SmsTimeStampRec*  timestamp,
                              int                     ref_num,
                              int                     pdu_count,
                              int                     pdu_index)
{
    int  coding;
    int  mtiByte  = 0x00;  /* message type - SMS DELIVER */

    if (pdu_count > 1)
        mtiByte |= 0x40;  /* user data header indicator */

    gsm_rope_add_c( rope, 0 );        /* no SC Address */
    gsm_rope_add_c( rope, mtiByte );     /* message type - SMS-DELIVER */
    gsm_rope_add_address( rope, sender_address );
    gsm_rope_add_c( rope, 0 );        /* protocol identifier */

    /* data coding scheme - GSM 7 bits / no class - or - 16-bit UCS2 / no class */
    coding = (use_gsm7 ? 0x00 : 0x08);

    gsm_rope_add_c( rope, coding );               /* data coding scheme       */
    gsm_rope_add_timestamp( rope, timestamp );    /* service center timestamp */

    if (use_gsm7) {
        bytes_t  dst;
        int    count = utf8_to_gsm7( utf8, utf8len, NULL, 0 );
        int    pad   = 0;

        if (pdu_count > 1)
        {
            assert( count <= MAX_USER_DATA_SEPTETS_WITH_HEADER );
            int  headerBits    = 6 * CHAR_BIT;  /* 6 is size of header in bytes */
            int  headerSeptets = headerBits / 7;
            if (headerBits % 7 > 0)
                headerSeptets += 1;

            pad = headerSeptets*7 - headerBits;
            gsm_rope_add_c( rope, count + headerSeptets );
            gsm_rope_add_sms_user_header(rope, ref_num, pdu_count, pdu_index);
        }
        else
        {
            assert( count <= MAX_USER_DATA_SEPTETS );
            gsm_rope_add_c( rope, count );
        }

        count = (count*7+pad+7)/8;  /* convert to byte count */

        dst = gsm_rope_reserve( rope, count );
        if (dst != NULL) {
            utf8_to_gsm7( utf8, utf8len, dst, pad );
        }
    } else {
        bytes_t  dst;
        int      count = utf8_to_ucs2( utf8, utf8len, NULL );


        if (pdu_count > 1)
        {
            assert( count*2 <= MAX_USER_DATA_BYTES_WITH_HEADER );
            gsm_rope_add_c( rope, count*2 + 6 );
            gsm_rope_add_sms_user_header( rope, ref_num, pdu_count, pdu_index );
        }
        else
        {
            assert( count*2 <= MAX_USER_DATA_BYTES );
            gsm_rope_add_c( rope, count*2 );
        }

        dst = gsm_rope_reserve( rope, count*2 );
        if (dst != NULL) {
            utf8_to_ucs2( utf8, utf8len, dst );
        }
    }
}


static SmsPDU
smspdu_create_deliver( cbytes_t               utf8,
                       int                    utf8len,
                       int                    use_gsm7,
                       const SmsAddressRec*   sender_address,
                       const SmsTimeStampRec* timestamp,
                       int                    ref_num,
                       int                    pdu_count,
                       int                    pdu_index )
{
    SmsPDU      p;
    GsmRopeRec  rope[1];
    int         size;

    p = calloc( sizeof(*p), 1 );
    if (!p) goto Exit;

    gsm_rope_init( rope );
    gsm_rope_add_sms_deliver_pdu( rope, utf8, utf8len, use_gsm7,
                                 sender_address, timestamp,
                                 ref_num, pdu_count, pdu_index);
    if (rope->error)
        goto Fail;

    gsm_rope_init_alloc( rope, rope->pos );

    gsm_rope_add_sms_deliver_pdu( rope, utf8, utf8len, use_gsm7,
                                 sender_address, timestamp,
                                 ref_num, pdu_count, pdu_index );

    p->base = gsm_rope_done_acquire( rope, &size );
    if (p->base == NULL)
        goto Fail;

    p->end  = p->base + size;
    p->tpdu = p->base + 1;
Exit:
    return p;

Fail:
    free(p);
    return NULL;
}


void
smspdu_free_list( SmsPDU*  pdus )
{
    if (pdus) {
        int  nn;
        for (nn = 0; pdus[nn] != NULL; nn++)
            smspdu_free( pdus[nn] );

        free( pdus );
    }
}



SmsPDU*
smspdu_create_deliver_utf8( const unsigned char*   utf8,
                            int                    utf8len,
                            const SmsAddressRec*   sender_address,
                            const SmsTimeStampRec* timestamp )
{
    SmsTimeStampRec  ts0;
    int              use_gsm7;
    int              count, block;
    int              num_pdus = 0;
    int              leftover = 0;
    SmsPDU*          list = NULL;

    static unsigned char  ref_num = 0;

    if (timestamp == NULL) {
        sms_timestamp_now( &ts0 );
        timestamp = &ts0;
    }

    /* can we encode the message with the GSM 7-bit alphabet ? */
    use_gsm7 = utf8_check_gsm7( utf8, utf8len );

    /* count the number of SMS PDUs we'll need */

    if (use_gsm7) {
        count = utf8_to_gsm7( utf8, utf8len, NULL, 0 );
        if (count > MAX_USER_DATA_SEPTETS) {
            block = MAX_USER_DATA_SEPTETS_WITH_HEADER;
        } else {
            block = MAX_USER_DATA_SEPTETS;
        }
        num_pdus = count / block;
        leftover = count - num_pdus*block;
    } else {
        count = utf8_to_ucs2( utf8, utf8len, NULL );
        if (count > MAX_USER_DATA_BYTES) {
            block = MAX_USER_DATA_BYTES_WITH_HEADER;
        } else {
            block = MAX_USER_DATA_BYTES;
        }
        num_pdus = (2 * count) / block;
        leftover = (2 * count) - num_pdus*block;
    }

    if (leftover > 0)
        num_pdus += 1;

    list = calloc( sizeof(SmsPDU*), num_pdus + 1 );
    if (list == NULL)
        return NULL;

    /* now create each SMS PDU */
    {
        cbytes_t   src     = utf8;
        cbytes_t   src_end = utf8 + utf8len;
        int        nn;

        for (nn = 0; nn < num_pdus; nn++)
        {
            int       skip = block;
            cbytes_t  src_next;

            if (leftover > 0 && nn == num_pdus-1)
                skip = leftover;

            /* How many utf8 symbols will fit in this PDU? */
            if (use_gsm7) {
                src_next = utf8_skip_gsm7( src, src_end, skip );
            } else {
                src_next = utf8_skip_ucs2( src, src_end, skip );
            }

            list[nn] = smspdu_create_deliver( src, src_next - src, use_gsm7, sender_address, timestamp,
                                              ref_num, num_pdus, nn );
            if (list[nn] == NULL)
                goto Fail;

            src = src_next;
        }
    }

    ref_num++;
    return list;

Fail:
    smspdu_free_list(list);
    return NULL;
}


SmsPDU
smspdu_create_from_hex( const char*  hex, int  hexlen )
{
    SmsPDU    p;
    cbytes_t  data;

    p = calloc( sizeof(*p), 1 );
    if (!p) goto Exit;

    p->base = malloc( (hexlen+1)/2 );
    if (p->base == NULL) {
        free(p);
        p = NULL;
        goto Exit;
    }

    if ( gsm_hex_to_bytes( (cbytes_t)hex, hexlen, p->base ) < 0 )
        goto Fail;

    p->end = p->base + (hexlen+1)/2;

    data = p->base;
    if ( sms_skip_sc_address( &data, p->end ) < 0 )
        goto Fail;

    p->tpdu = (bytes_t) data;

Exit:
    return p;

Fail:
    free(p->base);
    free(p);
    return NULL;
}

int
smspdu_to_hex( SmsPDU  pdu, char*  hex, int  hexlen )
{
    int  result = (pdu->end - pdu->base)*2;
    int  nn;

    if (hexlen > result)
        hexlen = result;

    for (nn = 0; nn*2 < hexlen; nn++) {
        gsm_hex_from_byte( &hex[nn*2], pdu->base[nn] );
    }
    return result;
}


/** SMS SUBMIT RECEIVER
 ** collects one or more SMS-SUBMIT PDUs to generate a single message to deliver
 **/

typedef struct SmsFragmentRec {
    struct SmsFragmentRec*  next;
    SmsAddressRec           from[1];
    byte_t                  ref;
    byte_t                  max;
    byte_t                  count;
    int                     index;
    SmsPDU*                 pdus;

} SmsFragmentRec, *SmsFragment;


typedef struct SmsReceiverRec {
    int           last;
    SmsFragment   fragments;

} SmsReceiverRec;


static void
sms_fragment_free( SmsFragment  frag )
{
    int  nn;

    for (nn = 0; nn < frag->max; nn++) {
        if (frag->pdus[nn] != NULL) {
            smspdu_free( frag->pdus[nn] );
            frag->pdus[nn] = NULL;
        }
    }
    frag->pdus  = NULL;
    frag->count = 0;
    frag->max   = 0;
    frag->index = 0;
    free( frag );
}

static SmsFragment
sms_fragment_alloc( SmsReceiver  rec, const SmsAddressRec*  from, int   ref, int  max )
{
    SmsFragment  frag = calloc(sizeof(*frag) + max*sizeof(SmsPDU), 1 );

    if (frag != NULL) {
        frag->from[0] = from[0];
        frag->ref     = ref;
        frag->max     = max;
        frag->pdus    = (SmsPDU*)(frag + 1);
        frag->index   = ++rec->last;
    }
    return  frag;
}



SmsReceiver   sms_receiver_create( void )
{
    SmsReceiver  rec = calloc(sizeof(*rec),1);
    return rec;
}

void
sms_receiver_destroy( SmsReceiver  rec )
{
    while (rec->fragments) {
        SmsFragment  frag = rec->fragments;
        rec->fragments = frag->next;
        sms_fragment_free(frag);
    }
}

static SmsFragment*
sms_receiver_find_p( SmsReceiver  rec, const SmsAddressRec*  from, int  ref )
{
    SmsFragment*  pnode = &rec->fragments;
    SmsFragment   node;

    for (;;) {
        node = *pnode;
        if (node == NULL)
            break;
        if (node->ref == ref && sms_address_eq( node->from, from ))
            break;
        pnode = &node->next;
    }
    return  pnode;
}

static SmsFragment*
sms_receiver_find_index_p( SmsReceiver  rec, int  index )
{
    SmsFragment*  pnode = &rec->fragments;
    SmsFragment   node;

    for (;;) {
        node = *pnode;
        if (node == NULL)
            break;
        if (node->index == index)
            break;
        pnode = &node->next;
    }
    return  pnode;
}

int
sms_receiver_add_submit_pdu( SmsReceiver  rec, SmsPDU       submit_pdu )
{
    SmsAddressRec  from[1];
    int            ref, max, cur;
    SmsFragment*   pnode;
    SmsFragment    frag;

    if ( smspdu_get_receiver_address( submit_pdu, from ) < 0 ) {
        D( "%s: could not extract receiver address\n", __FUNCTION__ );
        return -1;
    }

    ref = smspdu_get_ref( submit_pdu );
    if (ref < 0) {
        D( "%s: could not extract message reference from pdu\n", __FUNCTION__ );
        return -1;
    }
    max = smspdu_get_max_index( submit_pdu );
    if (max < 0) {
        D( "%s: invalid max fragment value: %d should be >= 1\n",
           __FUNCTION__, max );
        return -1;
    }
    pnode = sms_receiver_find_p( rec, from, ref );
    frag  = *pnode;
    if (frag == NULL) {
        frag = sms_fragment_alloc( rec, from, ref, max );
        if (frag == NULL) {
            D("%s: not enough memory to allocate new fragment\n", __FUNCTION__ );
            return -1;
        }
        if (android_telephony_debug_modem) {
            char  tmp[32];
            int   len;

            len = sms_address_to_str( from, tmp, sizeof(tmp) );
            if (len < 0) {
                strcpy( tmp, "<unknown>" );
                len = strlen(tmp);
            }
            D("%s: created SMS index %d, from %.*s, ref %d, max %d\n", __FUNCTION__,
               frag->index, len, tmp, frag->ref, frag->max);
        }
        *pnode = frag;
    }

    cur = smspdu_get_cur_index( submit_pdu );
    if (cur < 0) {
        D("%s: SMS fragment index is too small: %d should be >= 1\n", __FUNCTION__, cur+1 );
        return -1;
    }
    if (cur >= max) {
        D("%s: SMS fragment index is too large (%d >= %d)\n", __FUNCTION__, cur, max);
        return -1;
    }
    if ( frag->pdus[cur] != NULL ) {
        D("%s: receiving duplicate SMS fragment for %d/%d, ref=%d, discarding old one\n",
          __FUNCTION__, cur+1, max, ref);
        smspdu_free( frag->pdus[cur] );
        frag->count -= 1;
    }
    frag->pdus[cur] = submit_pdu;
    frag->count    += 1;

    if (frag->count >= frag->max) {
        /* yes, we received all fragments for this SMS */
        D( "%s: SMS index %d, received all %d fragments\n", __FUNCTION__, frag->index, frag->count );
        return frag->index;
    }
    else {
        /* still waiting for more */
        D( "%s: SMS index %d, received %d/%d, waiting for %d more\n", __FUNCTION__,
            frag->index, cur+1, max, frag->max - frag->count );
        return 0;
    }
}


int
sms_receiver_get_text_message( SmsReceiver  rec, int  index, bytes_t  utf8, int  utf8len )
{
    SmsFragment*  pnode = sms_receiver_find_index_p( rec, index );
    SmsFragment   frag  = *pnode;
    int           nn, total;

    if (frag == NULL) {
        D( "%s: invalid SMS index %d\n", __FUNCTION__, index );
        return -1;
    }
    if (frag->count != frag->max) {
        D( "%s: SMS index %d still needs %d fragments\n", __FUNCTION__,
           frag->index, frag->max - frag->count );
        return -1;
    }
    /* get the size of all combined text */
    total = 0;
    for ( nn = 0; nn < frag->count; nn++ ) {
        int  partial;
        if (utf8 && utf8len > 0) {
            partial  = smspdu_get_text_message( frag->pdus[nn], utf8, utf8len );
            utf8    += partial;
            utf8len -= partial;
        } else {
            partial  = smspdu_get_text_message( frag->pdus[nn], NULL, 0 );
        }
        total += partial;
    }
    return total;
}


static void
sms_receiver_remove( SmsReceiver  rec, int  index )
{
    SmsFragment*  pnode = sms_receiver_find_index_p( rec, index );
    SmsFragment   frag  = *pnode;
    if (frag != NULL) {
        *pnode = frag->next;
        sms_fragment_free(frag);
    }
}


SmsPDU*
sms_receiver_create_deliver( SmsReceiver  rec, int  index, const SmsAddressRec*  from )
{
    SmsPDU*          result = NULL;
    SmsFragment*     pnode = sms_receiver_find_index_p( rec, index );
    SmsFragment      frag  = *pnode;
    SmsTimeStampRec  now[1];
    int              nn, total;
    bytes_t          utf8;
    int              utf8len;

    if (frag == NULL) {
        D( "%s: invalid SMS index %d\n", __FUNCTION__, index );
        return NULL;
    }
    if (frag->count != frag->max) {
        D( "%s: SMS index %d still needs %d fragments\n", __FUNCTION__,
           frag->index, frag->max - frag->count );
        return NULL;
    }

    /* get the combined text message */
    utf8len = sms_receiver_get_text_message( rec, index, NULL, 0 );
    if (utf8len < 0)
        goto Exit;

    utf8 = malloc( utf8len + 1 );
    if (utf8 == NULL) {
        D( "%s: not enough memory to allocate %d bytes\n",
           __FUNCTION__, utf8len+1 );
        goto Exit;
    }

    total = 0;
    for ( nn = 0; nn < frag->count; nn++ ) {
        total += smspdu_get_text_message( frag->pdus[nn], utf8 + total, utf8len - total );
    }

    sms_timestamp_now( now );

    result = smspdu_create_deliver_utf8( utf8, utf8len, from, now );

    free(utf8);

Exit:
    sms_receiver_remove( rec, index );
    return result;
}
