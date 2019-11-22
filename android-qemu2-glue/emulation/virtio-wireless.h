/*
 * Virtio Wireless Device
 *
 */

#ifndef QEMU_VIRTIO_WIRELESS_H
#define QEMU_VIRTIO_WIRELESS_H

#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_WIRELESS "virtio-wireless-device"
#define VIRTIO_WIRELESS(obj) \
        OBJECT_CHECK(VirtIOWireless, (obj), TYPE_VIRTIO_WIRELESS)

typedef struct VirtIOWirelessQueue {
    VirtQueue *rx_vq;
    VirtQueue *tx_vq;
    QEMUBH *tx_bh;
    uint32_t tx_waiting;
    struct {
        VirtQueueElement *elem;
    } async_tx;
    struct VirtIOWireless *n;
} VirtIOWirelessQueue;

typedef struct LoopIo LoopIo;

typedef struct VirtIOWireless {
    VirtIODevice parent_obj;
    int sock;
    LoopIo* loop_io;
    uint8_t mac[ETH_ALEN];
    uint16_t status;
    int32_t tx_burst;
    VirtIOWirelessQueue *vqs;
    VirtQueue *ctrl_vq;
    NICState *nic;
    NICConf nic_conf;
    uint32_t has_vnet_hdr;
    size_t host_hdr_len;
    size_t guest_hdr_len;
    uint64_t host_features;
    DeviceState *qdev;
} VirtIOWireless;

// This function is necessary because we need to make sure virtio-wireless object is 
// linked to the final executable and type_init(virtio_register_types) gets called. 
// TODO(wdu@) Find a better solution.
extern void set_status(VirtIOWireless* n, int status);
#endif
