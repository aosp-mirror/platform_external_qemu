/* Copyright 2015 The Android Open Source Project
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

// Shared type and constant declarations between android_pipe.h and
// android_pipe_host.h

// Buffer descriptor for android_pipe_send() and android_pipe_recv().
typedef struct AndroidPipeBuffer {
    uint8_t* data;
    size_t size;
} AndroidPipeBuffer;

/* Possible status values used to signal errors - see qemu_pipe_error_convert */
#define PIPE_ERROR_INVAL       -1
#define PIPE_ERROR_AGAIN       -2
#define PIPE_ERROR_NOMEM       -3
#define PIPE_ERROR_IO          -4

/* Bit-flags used to signal events from the emulator */
#define PIPE_WAKE_CLOSED       (1 << 0)  /* emulator closed pipe */
#define PIPE_WAKE_READ         (1 << 1)  /* pipe can now be read from */
#define PIPE_WAKE_WRITE        (1 << 2)  /* pipe can now be written to */

/* List of bitflags returned in status of CMD_POLL command */
#define PIPE_POLL_IN   (1 << 0)   /* means guest can read */
#define PIPE_POLL_OUT  (1 << 1)   /* means guest can write */
#define PIPE_POLL_HUP  (1 << 2)   /* means closed by host */

////////////////////////////////////////////////////////////////////////////
//
// The following functions are called from the host and must be implemented
// by the virtual device, through a series of callbacks that are registered
// by calling android_pipe_set_hw_funcs() declared below.
//
// IMPORTANT: These must be called in the context of the 'device thread' only.
// See technical note in android_pipe_device.h for more details.

/* This tells the guest system that we want to close the pipe and that
 * further attempts to read or write to it will fail. This will not
 * necessarily destroys the |hwpipe| immediately. The latter will call
 * android_pipe_free() at destruction time though.
 *
 * This will also wake-up any blocked guest threads waiting for i/o.
 */
extern void android_pipe_close(void* hwpipe);

/* Signal that the pipe can be woken up. 'flags' must be a combination of
 * PIPE_WAKE_READ and PIPE_WAKE_WRITE.
 */
extern void android_pipe_wake(void* hwpipe, unsigned flags);

ANDROID_END_HEADER
