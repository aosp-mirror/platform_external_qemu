/* Copyright (C) 2010 The Android Open Source Project
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

#ifndef _ANDROID_PROTOCOL_UI_COMMON_H
#define _ANDROID_PROTOCOL_UI_COMMON_H

/*
 * Contains declarations for UI control protocol used by both the Core, and the
 * UI.
 */

/* UI control command header.
 * Every UI control command sent by the Core, or by the UI begins with this
 * header, immediatelly followed by the command parameters (if there are any).
 * Command type is defined by cmd_type field of this header. If command doesn't
 * have any command-specific parameters, cmd_param_size field of this header
 * must be set to 0.
 */
typedef struct UICmdHeader {
    /* Command type. */
    uint8_t     cmd_type;

    /* Byte size of the buffer containing parameters for the comand defined by
     * the cmd_type field. The buffer containing parameters must immediately
     * follow this header. If command doesn't have any parameters, this field
     * must be set to 0 */
    uint32_t    cmd_param_size;
} UICmdHeader;

/* UI control command response header.
 * If UI control command assumes a response from the remote end, the response
 * must start with this header, immediately followed by the response data buffer.
 */
typedef struct UICmdRespHeader {
    /* Result of the command handling. */
    int result;

    /* Byte size of the buffer containing response data following this header. If
     * there are no response data for the commad, this field must be set to 0. */
    uint32_t    resp_data_size;
} UICmdRespHeader;

/* Calculates timeout for transferring the given number of bytes via UI control
 * socket.
 * Return:
 *  Number of milliseconds during which the entire number of bytes is expected
 *  to be transferred.
 */
// TODO: Move to more appropriate place!
static inline int
_get_transfer_timeout(size_t data_size)
{
    // Min 200 millisec + one millisec for each transferring byte.
    // TODO: Come up with a better arithmetics here.
    return 2000 + data_size;
}

#endif /* _ANDROID_PROTOCOL_UI_COMMON_H */
