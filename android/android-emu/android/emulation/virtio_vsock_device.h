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

#include <stdint.h>

struct IVsockHostCallbacks {
    virtual ~IVsockHostCallbacks() {}
    virtual void onConnect() = 0;
    virtual void onReceive(const void *data, size_t size) = 0;
    virtual void onClose() = 0;
};

typedef uint64_t(*virtio_vsock_device_stream_open_t)(uint32_t guest_port,
                                                     IVsockHostCallbacks *callbacks);
typedef size_t(*virtio_vsock_device_stream_send_t)(uint64_t key,
                                                   const void *data, size_t size);
typedef bool(*virtio_vsock_device_stream_ping_t)(uint64_t key);
typedef bool(*virtio_vsock_device_stream_close_t)(uint64_t key);

struct virtio_vsock_device_ops_t {
    virtio_vsock_device_stream_open_t   open;
    virtio_vsock_device_stream_send_t   send;
    virtio_vsock_device_stream_ping_t   ping;
    virtio_vsock_device_stream_close_t  close;
};

const virtio_vsock_device_ops_t *
virtio_vsock_device_set_ops(const virtio_vsock_device_ops_t *ops);

const virtio_vsock_device_ops_t *virtio_vsock_device_get_host_ops();
