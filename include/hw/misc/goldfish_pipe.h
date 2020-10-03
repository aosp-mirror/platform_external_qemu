/* Copyright (C) 2016 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_PIPE_H
#define _HW_GOLDFISH_PIPE_H

#include "qemu/typedefs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The Android pipe virtual device expects an implementation of
 * pipe services to be provided according to the following interface
 */
typedef struct GoldfishPipeBuffer {
    void* data;
    size_t size;
} GoldfishPipeBuffer;

/* List of bitflags returned by guest_poll() */
typedef enum {
    GOLDFISH_PIPE_POLL_IN = (1 << 0),   /* means guest can read */
    GOLDFISH_PIPE_POLL_OUT = (1 << 1),  /* means guest can write */
    GOLDFISH_PIPE_POLL_HUP = (1 << 2),  /* means closed by host */
} GoldfishPipePollFlags;

/* List of bitflags used to call goldfish_pipe_signal_wake() and
 * guest_wake_on() */
typedef enum {
    GOLDFISH_PIPE_WAKE_CLOSED = (1 << 0),     /* emulator closed the pipe */
    GOLDFISH_PIPE_WAKE_READ = (1 << 1),       /* pipe can now be read from */
    GOLDFISH_PIPE_WAKE_WRITE = (1 << 2),      /* pipe can now be written to */
    GOLDFISH_PIPE_WAKE_UNLOCK_DMA  = (1 << 3),/* unlock this pipe's DMA buffer */
} GoldfishPipeWakeFlags;

/* List of error values possibly returned by guest_recv() and
 * guest_send(). */
typedef enum {
    GOLDFISH_PIPE_ERROR_INVAL = -1,
    GOLDFISH_PIPE_ERROR_AGAIN = -2,
    GOLDFISH_PIPE_ERROR_NOMEM = -3,
    GOLDFISH_PIPE_ERROR_IO = -4,
} GoldfishPipeError;

/* List of reasons why pipe is closed. */
typedef enum {
    GOLDFISH_PIPE_CLOSE_GRACEFUL = 0,
    GOLDFISH_PIPE_CLOSE_REBOOT = 1,
    GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT = 2,
    GOLDFISH_PIPE_CLOSE_ERROR = 3,
} GoldfishPipeCloseReason;

/* Opaque type of the hardware-side view of a pipe connection. This structure
 * is implemented by the virtual device and is hidden from the host service
 * implementation. */
typedef struct GoldfishHwPipe GoldfishHwPipe;

/* Opaque type of the host-side view of a pipe connection. This structure is
 * implemented by the host pipe service implementation, and is hidden from
 * the virtual device. */
typedef struct GoldfishHostPipe GoldfishHostPipe;

typedef struct {
    // Open a new pipe. |hw_pipe| is a unique pointer value identifying the
    // hardware-side view of the pipe, and will be passed to the
    // goldfish_pipe_xxx() functions below. This returns a new host-specific
    // pipe value that is only used by the virtual device to call other
    // callbacks here. There is a one-to-one association between a |hw_pipe|
    // and its |host_pipe| value, which can be reset by calling
    // goldfish_pipe_reset().
    GoldfishHostPipe* (*guest_open)(GoldfishHwPipe *hw_pipe);
    GoldfishHostPipe* (*guest_open_with_flags)(GoldfishHwPipe *hw_pipe, uint32_t flags);

    // Close and free a pipe. |host_pipe| must be the result of a previous
    // guest_open() or guest_load() call, or the second parameter to
    // goldfish_pipe_reset(). |reason| is why the pipe was closed.
    void (*guest_close)(GoldfishHostPipe *host_pipe,
                        GoldfishPipeCloseReason reason);

    // A set of hooks to prepare and cleanup save/load operations. These are
    // called once per each snapshot operation, as opposed to the following
    // guest_load()/guest_save().
    void (*guest_pre_load)(QEMUFile *file);
    void (*guest_post_load)(QEMUFile *file);
    void (*guest_pre_save)(QEMUFile *file);
    void (*guest_post_save)(QEMUFile *file);

    // Load the state of a  pipe from a stream. |file| is the input stream,
    // |hw_pipe| is the hardware-side pipe descriptor. On success, return a new
    // internal pipe instance (similar to one returned by guest_open()), and
    // sets |*force_close| to 1 to indicate that the pipe must be force-closed
    // just after its load/creation (only useful for certain services that can't
    // preserve state into streams). Return NULL on faillure.
    GoldfishHostPipe* (*guest_load)(QEMUFile *file, GoldfishHwPipe *hw_pipe,
                                    char *force_close);

    // Save the state of a pipe to a stream. |host_pipe| is the pipe
    // instance from guest_open() or guest_load(). and |file| is the
    // output stream.
    void (*guest_save)(GoldfishHostPipe *host_pipe, QEMUFile *file);

    // Poll the state of the pipe associated with |host_pipe|.
    // This returns a combination of GoldfishPipePollFlags.
    GoldfishPipePollFlags (*guest_poll)(GoldfishHostPipe *host_pipe);

    // Called when the guest tries to receive data from the host through
    // |host_pipe|. This will try to copy data to the memory ranges
    // decribed by the array |buffers| or |num_buffers| items.
    // Return number of bytes transferred, or a (negative) GoldfishPipeError
    // value otherwise.
    int (*guest_recv)(GoldfishHostPipe *host_pipe,
                      GoldfishPipeBuffer *buffers,
                      int num_buffers);

    // Called when the guest tries to send data to the host through
    // |host_pipe|. This will try to copy data from the memory ranges
    // decribed by the array |buffers| or |num_buffers| items.
    // Return number of bytes transferred, or a (negative) GoldfishPipeError
    // value otherwise.
    int (*guest_send)(GoldfishHostPipe **host_pipe,
                      const GoldfishPipeBuffer *buffers,
                      int num_buffers);

    // Called when the guest wants to be waked on specific events.
    // identified by the |wake_flags| bitmask. The host should call
    // goldfish_pipe_signal_wake() with the appropriate bitmask when
    // any of these events occur.
    void (*guest_wake_on)(GoldfishHostPipe *host_pipe,
                          GoldfishPipeWakeFlags wake_flags);

    // called to register a new DMA buffer that can be mapped in guest + host.
    void (*dma_add_buffer)(void* pipe, uint64_t guest_paddr, uint64_t);
    // called when the guest is done with a particular DMA buffer.
    void (*dma_remove_buffer)(uint64_t guest_paddr);
    // called when host mappings change, such
    // as on pipe save/load during snapshots.
    void (*dma_invalidate_host_mappings)(void);
    // called when device reboots and we need to reset pipe state completely.
    void (*dma_reset_host_mappings)(void);
    // For snapshot save/load of DMA buffer state.
    void (*dma_save_mappings)(QEMUFile* file);
    void (*dma_load_mappings)(QEMUFile* file);
} GoldfishPipeServiceOps;

/* Called by the service implementation to register its callbacks.
 * The default implementation doesn't do anything except returning
 * an error when |guest_open| or |guest_load| are called. */
extern void goldfish_pipe_set_service_ops(
        const GoldfishPipeServiceOps* ops);

/* Query the service ops struct from other places. For use with
 * virtio. */
extern const GoldfishPipeServiceOps* goldfish_pipe_get_service_ops(void);

/* Function to look up id of hwpipe and vice versa. */
extern int goldfish_pipe_get_id(GoldfishHwPipe* hw_pipe);
extern GoldfishHwPipe* goldfish_pipe_lookup_by_id(int id);

/* Implemented by the virtual device, always called from the service in
 * a thread that owns the BQL. */

/* Reset the association of |hw_pipe| with a new |host_pipe|
 * value. This is called once the guest has written the service name
 * to the initial pipe connection, and the host decided of the right
 * implementation for it. */
extern void goldfish_pipe_reset(GoldfishHwPipe *hw_pipe,
                                GoldfishHostPipe *host_pipe);

/* Called by the host to notify the virtual device that it has
 * closed its side of the pipe. */
extern void goldfish_pipe_close_from_host(GoldfishHwPipe *hw_pipe);

/* Called by the host to notify that one or more conditions identified
 * by |flags| has been met for a given pipe identified by |hw_pipe|. */
extern void goldfish_pipe_signal_wake(GoldfishHwPipe *hw_pipe,
                                      GoldfishPipeWakeFlags flags);

#endif /* _HW_GOLDFISH_PIPE_H */
