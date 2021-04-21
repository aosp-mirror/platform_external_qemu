/* Copyright 2021 The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at

** http://www.apache.org/licenses/LICENSE-2.0

** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** A virtio device implementing a hardware gnss device.
**
*/

#ifndef QEMU_VIRTIO_GNSS_H
#define QEMU_VIRTIO_GNSS_H
#include <stdint.h>
#include "qemu/compiler.h"
#include "qemu/osdep.h"
#include "qemu/typedefs.h"
#include "hw/virtio/virtio.h"
#include "qapi/error.h"

#define TYPE_VIRTIO_GNSS "virtio-gnss-device"
#define VIRTIO_GNSS(obj) OBJECT_CHECK(VirtIOGNSS, (obj), TYPE_VIRTIO_GNSS)
#define VIRTIO_GNSS_GET_PARENT_CLASS(obj)                                      \
    OBJECT_GET_PARENT_CLASS(obj, TYPE_VIRTIO_GNSS)

typedef struct VirtIOGNSS {
    VirtIODevice parent_obj;

    /* input to guest device */
    VirtQueue *ivq;

    /* output from guest device to host backend */
    VirtQueue *ovq;

    uint32_t xtype;

    VMChangeStateEntry *vmstate;
} VirtIOGNSS;

#endif
