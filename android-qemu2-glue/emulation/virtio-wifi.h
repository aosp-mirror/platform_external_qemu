/*
 * Virtio Wireless Device
 *
 */

#ifndef QEMU_VIRTIO_WIFI_H
#define QEMU_VIRTIO_WIFI_H

#ifdef __cplusplus
extern "C" {
#else
#endif

#include "standard-headers/linux/if_ether.h"
#define TYPE_VIRTIO_WIFI "virtio-wifi-device"
#define VIRTIO_WIFI(obj) OBJECT_CHECK(VirtIOWifi, (obj), TYPE_VIRTIO_WIFI)

typedef struct LoopIo LoopIo;

typedef struct VirtIOWifiQueue {
    VirtQueue* rx;
    VirtQueue* tx;
    VirtQueue* tx_p2p;
    QEMUBH* tx_bh;
    QEMUBH* tx_p2p_bh;
    uint32_t tx_waiting;
    struct {
        VirtQueueElement* elem;
    } async_tx;
    struct VirtIOWifi* n;
} VirtIOWifiQueue;

typedef struct VirtIOWifi {
    VirtIODevice parent_obj;
    uint8_t mac[ETH_ALEN];
    uint16_t status;
    int32_t tx_burst;
    VirtIOWifiQueue* vqs;
    VirtQueue* ctrl_vq;
    NICConf nic_conf;
    uint32_t has_vnet_hdr;
    size_t host_hdr_len;
    size_t guest_hdr_len;
    uint64_t host_features;
    DeviceState* qdev;
} VirtIOWifi;

// If multiple emulators are being used, we will need to set different mac
// address prefix so that mac addresses won't clash.
extern void virtio_wifi_set_mac_prefix(int prefix);

#ifdef __cplusplus
}
#else
#endif

#endif