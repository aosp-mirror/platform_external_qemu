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
    //QEMUTimer *tx_timer;
    //QEMUBH *tx_bh;
    uint32_t tx_waiting;
    struct {
        VirtQueueElement *elem;
    } async_tx;
    struct VirtIOWireless *n;
} VirtIOWirelessQueue;

typedef struct LoopIo LoopIo;

typedef struct VirtIOWireless {
    VirtIODevice parent_obj;
    uint8_t mac[ETH_ALEN];
    uint16_t status;
    VirtIOWirelessQueue *vqs;
    VirtQueue *ctrl_vq;
    NICState *nic;
    NICConf nic_conf;
    uint64_t host_features;
    int sock;
    LoopIo* loop_io;
    DeviceState *qdev;
} VirtIOWireless;
extern void set_status(VirtIOWireless* n, int status);
#endif
