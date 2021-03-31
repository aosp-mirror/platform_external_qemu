/*
 * QEMU IPMI SMBus (SSIF) emulation
 *
 * Copyright (c) 2015,2016 Corey Minyard, MontaVista Software, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef SMBUS_IPMI_H
#define SMBUS_IPMI_H

#define SSIF_IPMI_REQUEST                       2
#define SSIF_IPMI_MULTI_PART_REQUEST_START      6
#define SSIF_IPMI_MULTI_PART_REQUEST_MIDDLE     7
#define SSIF_IPMI_MULTI_PART_REQUEST_END        8
#define SSIF_IPMI_RESPONSE                      3
#define SSIF_IPMI_MULTI_PART_RESPONSE_MIDDLE    9
#define SSIF_IPMI_MULTI_PART_RETRY              0xa

#define MAX_SSIF_IPMI_MSG_SIZE 255
#define MAX_SSIF_IPMI_MSG_CHUNK 32

#define IPMI_GET_SYS_INTF_CAP_CMD 0x57

#endif /* SMBUS_IPMI_H */
