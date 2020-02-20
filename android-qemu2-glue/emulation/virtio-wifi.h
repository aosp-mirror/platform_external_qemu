/*
 * Virtio Wireless Device
 *
 */

#ifndef QEMU_VIRTIO_WIFI_H
#define QEMU_VIRTIO_WIFI_H

#include "hw/virtio/virtio.h"
#include "standard-headers/linux/if_ether.h"

#define TYPE_VIRTIO_WIFI "virtio-wifi-device"
#define VIRTIO_WIFI(obj) OBJECT_CHECK(VirtIOWifi, (obj), TYPE_VIRTIO_WIFI)

typedef struct VirtIOWifiQueue {
    VirtQueue* rx_data;
    VirtQueue* rx_mgmt;
    VirtQueue* tx_vq;
    QEMUBH* tx_bh;
    uint32_t tx_waiting;
    struct {
        VirtQueueElement* elem;
    } async_tx;
    struct VirtIOWifi* n;
} VirtIOWifiQueue;

typedef struct LoopIo LoopIo;

typedef struct VirtIOWifi {
    VirtIODevice parent_obj;
    int sock;
    LoopIo* loop_io;
    uint8_t mac[ETH_ALEN];
    uint16_t status;
    int32_t tx_burst;
    VirtIOWifiQueue* vqs;
    VirtQueue* ctrl_vq;
    NICState* nic;
    NICConf nic_conf;
    uint32_t has_vnet_hdr;
    size_t host_hdr_len;
    size_t guest_hdr_len;
    uint64_t host_features;
    DeviceState* qdev;
} VirtIOWifi;

struct virtio_wifi_config {
    uint8_t mac[ETH_ALEN];
    /* Default maximum transmit unit advice */
    uint16_t mtu;
} QEMU_PACKED;
// This function is necessary because we need to make sure virtio-wifi object is
// linked to the final executable and type_init(virtio_register_types) gets
// called.
// TODO(wdu@) Find a better solution.
extern void set_status(VirtIOWifi* n, int status);
#endif
