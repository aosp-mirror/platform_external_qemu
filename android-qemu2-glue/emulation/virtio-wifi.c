#include "qemu/osdep.h"
#include "android-qemu2-glue/emulation/virtio-wifi.h"

#include "android/utils/looper.h"
#include "android/utils/sockets.h"
#include "hw/virtio/virtio-pci.h"
#include "net/net.h"
#include "qemu/iov.h"
#include "slirp/slirp.h"

// TODO (wdu@) For now, undef the following MACROs or functions to
// avoid name clashes and function redifinitions.
// Move iee80211 to 8023 conversion to its own file.
#undef ARRAY_SIZE
#define is_broadcast_ether_addr _is_broadcast_ether_addr
#define is_multicast_ether_addr _is_multicast_ether_addr
#include "common/ieee802_11_defs.h"
#undef is_broadcast_ether_addr
#undef is_multicast_ether_addr

#define TX_TIMER_INTERVAL 150000 /* 150 us */

/* Limit the number of packets that can be sent via a single flush
 * of the TX queue.  This gives us a guaranteed exit condition and
 * ensures fairness in the io path.  256 conveniently matches the
 * length of the TX queue and shows a good balance of performance
 * and latency. */
#define TX_BURST 256

/* Maximum packet size we can receive from virtio-wifi driver */
#define IEEE80211_MAX_FRAME_LEN 2352

/* previously fixed value */
#define VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE 256
#define VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE 256

/* for now, only allow larger queues; with virtio-1, guest can downsize */
#define VIRTIO_WIFI_RX_QUEUE_MIN_SIZE VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE
#define VIRTIO_WIFI_TX_QUEUE_MIN_SIZE VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE

static int32_t virtio_wifi_flush_tx(VirtIOWifiQueue* q);

static int vq2q(int queue_index) {
    return queue_index / 2;
}

static void peer_test_vnet_hdr(VirtIOWifi* n) {
    NetClientState* nc = qemu_get_queue(n->nic);
    if (!nc->peer) {
        return;
    }

    n->has_vnet_hdr = qemu_has_vnet_hdr(nc->peer);
}

static int virtio_wifi_can_receive(NetClientState* nc) {
    VirtIOWifi* n = qemu_get_nic_opaque(nc);
    VirtIODevice* vdev = VIRTIO_DEVICE(n);

    if (!vdev->vm_running) {
        return 0;
    }
    if (!virtio_queue_ready(n->vqs->rx_data) ||
        !(vdev->status & VIRTIO_CONFIG_S_DRIVER_OK)) {
        return 0;
    }

    return 1;
}

// set virtio-wifi link status according to netclientstate
static void virtio_wifi_set_link_status(NetClientState* nc) {}

static int virtio_wifi_has_buffers(VirtQueue* vq) {
    if (virtio_queue_empty(vq)) {
        virtio_queue_set_notification(vq, 1);

        /* To avoid a race condition where the guest has made some buffers
         * available after the above check but before notification was
         * enabled, check for available buffers again.
         */
        if (virtio_queue_empty(vq)) {
            return 0;
        }
    }

    virtio_queue_set_notification(vq, 0);
    return 1;
}

static ssize_t virtio_wifi_add_buf(VirtIODevice* vdev,
                                   VirtQueue* vq,
                                   const uint8_t* buf,
                                   size_t size) {
    VirtQueueElement* elem;
    struct iovec* sg;
    size_t guest_offset, offset;
    offset = guest_offset = 0;
    if (!virtio_wifi_has_buffers(vq/*, size + ETH_HLEN + n->guest_hdr_len - n->host_hdr_len*/)) {
        fprintf(stderr, "%s virtio-wifi is empty!\n", __func__);
        return 0;
    }

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    if (!elem) {
        fprintf(stderr, "%s virtio-wifi unexpected empty queue\n", __func__);
        return -1;
    }
    sg = elem->in_sg;
    size_t len = iov_from_buf(sg, elem->in_num, guest_offset, buf + offset,
                              size - offset);

    /* signal other side */
    virtqueue_fill(vq, elem, len, 0);
    g_free(elem);
    virtqueue_flush(vq, 1);
    virtio_notify(vdev, vq);
    return size;
}

static int is_frame_data(void* buf) {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)buf;
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_DATA;
}

/*test if the mac80211 frame is a toDS data frame*/
static int is_frame_tods(void* buf) {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)buf;
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_DATA &&
           (hdr->frame_control & WLAN_FC_TODS);
}

static int ieee8021_is_data_qos(void* buf) {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)buf;
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_DATA &&
           (hdr->frame_control & (WLAN_FC_STYPE_QOS_DATA << 4));
}
static int is_frame_fromds(void* buf) {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)buf;
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_DATA &&
           (hdr->frame_control & WLAN_FC_FROMDS);
}

/* See IEEE 802.1H for LLC/SNAP encapsulation/decapsulation */
/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
static const unsigned char rfc1042_header[] = {0xaa, 0xaa, 0x03,
                                               0x00, 0x00, 0x00};

/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
static const unsigned char bridge_tunnel_header[] = {0xaa, 0xaa, 0x03,
                                                     0x00, 0x00, 0xf8};

static const unsigned char s_bssid[] = {0x00, 0x13, 0x10, 0x85, 0xfe, 0x01};

static ssize_t ieee80211_data_from_8023(const uint8_t* buf,
                                        VirtQueueElement* elem,
                                        size_t orig_size) {
    struct ieee80211_hdr hdr;
    le16 ethertype, fc;
    const u8* encaps_data;
    struct iovec* sg;
    size_t encaps_len, skip_header_bytes, total;
    size_t hdrlen = IEEE80211_HDRLEN;
    if (orig_size < ETH_HLEN) {
        return 0;
    }
    ethertype = (buf[12] << 8) | buf[13];
    skip_header_bytes = ETH_HLEN;
    if (ethertype >= 0x0600) {
        encaps_data = rfc1042_header;
        encaps_len = sizeof(rfc1042_header);
        skip_header_bytes -= 2;
    } else {
        encaps_data = NULL;
        encaps_len = 0;
    }
    fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    fc |= WLAN_FC_FROMDS;
    /* DA BSSID SA */
    memcpy(hdr.addr1, buf, ETH_ALEN);
    memcpy(hdr.addr2, s_bssid, ETH_ALEN);
    memcpy(hdr.addr3, buf + ETH_ALEN, ETH_ALEN);
    hdr.frame_control = fc;
    hdr.duration_id = 0;
    hdr.seq_ctrl = 0;
    sg = elem->in_sg;
    total = iov_from_buf(sg, elem->in_num, 0, (const void*)&hdr,
                         IEEE80211_HDRLEN);
    if (encaps_len) {
        total +=
                iov_from_buf(sg, elem->in_num, hdrlen, encaps_data, encaps_len);
    }
    total += iov_from_buf(sg, elem->in_num, hdrlen + encaps_len,
                          buf + skip_header_bytes,
                          orig_size - skip_header_bytes);
    return total;
}

// Assuming ethernet frames from VNIC is always data frames so we can safely
// forward them back to the guest.
static ssize_t virtio_wifi_receive_rcu(NetClientState* nc,
                                       const uint8_t* buf,
                                       size_t size) {
    VirtIOWifi* n = qemu_get_nic_opaque(nc);
    VirtIOWifiQueue* q = &n->vqs[0];
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    VirtQueueElement* elem;
    ssize_t offset = 0;
    if (!virtio_wifi_can_receive(nc)) {
        return -1;
    }
    if (!virtio_wifi_has_buffers(q->rx_data/*, size + ETH_HLEN + n->guest_hdr_len - n->host_hdr_len*/)) {
        fprintf(stderr, "%s virtio-wifi is empty!\n", __func__);
        return 0;
    }
    while (offset < size) {
        elem = virtqueue_pop(q->rx_data, sizeof(VirtQueueElement));
        if (!elem) {
            fprintf(stderr, "%s virtio-wifi unexpected empty queue\n",
                    __func__);
            return -1;
        }
        if (elem->in_num < 1) {
            fprintf(stderr, "virtio-wifi receive queue contains no in buffers");
            virtqueue_detach_element(q->rx_data, elem, 0);
            g_free(elem);
            return -1;
        }
        offset = ieee80211_data_from_8023(buf, elem, size);
        if (offset) {
            /* signal other side */
            virtqueue_fill(q->rx_data, elem, offset, 0);
            g_free(elem);
            virtqueue_flush(q->rx_data, 1);
            virtio_notify(vdev, q->rx_data);
        } else {
            virtqueue_unpop(q->rx_data, elem, offset);
            g_free(elem);
        }
    }
    return offset;
}

static ssize_t virtio_wifi_receive(NetClientState* nc,
                                   const uint8_t* buf,
                                   size_t size) {
    ssize_t r;
    rcu_read_lock();
    r = virtio_wifi_receive_rcu(nc, buf, size);
    rcu_read_unlock();
    return r;
}

static NetClientInfo wifi_virtio_info = {
        .type = NET_CLIENT_DRIVER_NIC,
        .size = sizeof(NICState),
        .can_receive = virtio_wifi_can_receive,
        .receive = virtio_wifi_receive,
        .link_status_changed = virtio_wifi_set_link_status,
        //.query_rx_filter = virtio_net_query_rxfilter,
};

static struct iovec* ieee80211_data_to_8023(struct iovec* out_sg,
                                            unsigned int* out_num,
                                            struct iovec* sg) {
    u16 hdrlen = sizeof(rfc1042_header) + IEEE80211_HDRLEN;
    u16 ethertype;
    u8 dst[ETH_ALEN];
    u8 src[ETH_ALEN];
    uint8_t buf[64];
    iov_to_buf(out_sg, *out_num, 0, buf, 64);
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)buf;
    // Null Data TODO(wdu@) Find the correct constant
    if (hdr->frame_control & (1 << 6)) {
        return NULL;
    }
    if (is_frame_tods(buf) && !is_frame_fromds(buf)) {
    } else {
        return NULL;
    }
    memcpy(dst, hdr->addr3, ETH_ALEN);
    memcpy(src, hdr->addr2, ETH_ALEN);

    ethertype = (buf[hdrlen] << 8) | buf[hdrlen + 1];
    if (ethertype != ETH_P_IPV6 && ethertype != ETH_P_IP &&
        ethertype != ETH_P_ARP && ethertype != ETH_P_NCSI) {
        fprintf(stderr, "## unknown ethertype %d\n", ethertype);
        for (int i = 0; i < 64; i++) {
            fprintf(stderr, "%x ", buf[i]);
        }
        fprintf(stderr, "\n");
        return NULL;
    }
    struct eth_header* eth = g_malloc0(sizeof(struct eth_header));
    eth->h_proto = htons(ethertype);
    memcpy(eth->h_dest, dst, ETH_ALEN);
    memcpy(eth->h_source, src, ETH_ALEN);
    sg[0].iov_base = eth;
    sg[0].iov_len = sizeof(struct eth_header);
    *out_num = iov_copy(&sg[1], VIRTQUEUE_MAX_SIZE, out_sg, *out_num,
                        hdrlen + sizeof(eth->h_proto), iov_size(out_sg, *out_num)) +
               1;
    return sg;
}

static void virtio_wifi_tx_complete(NetClientState* nc, ssize_t len) {
    VirtIOWifi* n = qemu_get_nic_opaque(nc);
    VirtIOWifiQueue* q = n->vqs;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    if (q->async_tx.elem) {
        virtqueue_push(q->tx_vq, q->async_tx.elem, 0);
        virtio_notify(vdev, q->tx_vq);
        g_free(q->async_tx.elem);
        q->async_tx.elem = NULL;
    }
    virtio_queue_set_notification(q->tx_vq, 1);
    virtio_wifi_flush_tx(q);
}

static int32_t virtio_wifi_flush_tx(VirtIOWifiQueue* q) {
    VirtIOWifi* n = q->n;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    VirtQueueElement* elem;
    struct ieee80211_hdr hdr;
    int32_t num_packets = 0;
    int32_t ret = 0;

    if (q->async_tx.elem) {
        virtio_queue_set_notification(q->tx_vq, 0);
        return num_packets;
    }

    for (;;) {
        ssize_t ret;
        unsigned int out_num;
        struct iovec sg[VIRTQUEUE_MAX_SIZE + 1], *out_sg;
        elem = virtqueue_pop(q->tx_vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        out_num = elem->out_num;
        out_sg = elem->out_sg;
        if (out_num < 1) {
            fprintf(stderr, "virtio-wifi header not in first element");
            virtqueue_detach_element(q->tx_vq, elem, 0);
            g_free(elem);
            return -EINVAL;
        }
        size_t s = iov_size(out_sg, out_num);

        if (s >= IEEE80211_HDRLEN) {
            iov_to_buf(out_sg, out_num, 0, &hdr, IEEE80211_HDRLEN);
            if (is_frame_data((void*)&hdr)) {
                out_sg = ieee80211_data_to_8023(out_sg, &out_num, sg);
                if (out_sg) {
                    ret = qemu_sendv_packet_async(qemu_get_subqueue(n->nic, 0),
                                                  out_sg, out_num,
                                                  virtio_wifi_tx_complete);
                    if (ret == 0) {
                        virtio_queue_set_notification(q->tx_vq, 0);
                        q->async_tx.elem = elem;
                        return -EBUSY;
                    }
                }
            } else {
                uint8_t buf[IEEE80211_MAX_FRAME_LEN];
                // TODO (wdu@) Experiement with shared memory approach
                size_t size = iov_to_buf(out_sg, out_num, 0, buf,
                                         IEEE80211_MAX_FRAME_LEN);
                send(n->sock, buf, size, 0);
            }
        }
        virtqueue_push(q->tx_vq, elem, 0);
        virtio_notify(vdev, q->tx_vq);
        g_free(elem);
        if (++num_packets >= n->tx_burst) {
            break;
        }
    }
    return num_packets;
}

static void virtio_wifi_handle_tx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    VirtIOWifiQueue* q = n->vqs;
    if (unlikely(n->status & VIRTIO_NET_S_LINK_UP) == 0) {
        // TODO (wdu@) Need to drop all data in queue
        return;
    }

    if (unlikely(q->tx_waiting)) {
        return;
    }
    q->tx_waiting = 1;
    // This happens when device was stopped but VCPU wasn't.
    if (!vdev->vm_running) {
        return;
    }
    virtio_queue_set_notification(vq, 0);
    qemu_bh_schedule(q->tx_bh);
}

static void virtio_wifi_tx_bh(void* opaque) {
    VirtIOWifiQueue* q = opaque;
    VirtIOWifi* n = q->n;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    int32_t ret;

    /* This happens when device was stopped but BH wasn't. */
    if (!vdev->vm_running) {
        /* Make sure tx waiting is set, so we'll run when restarted. */
        assert(q->tx_waiting);
        return;
    }

    q->tx_waiting = 0;

    ret = virtio_wifi_flush_tx(q);
    if (ret == -EBUSY || ret == -EINVAL) {
        return; /* Notification re-enable handled by tx_complete or device
                 * broken */
    }

    /* If we flush a full burst of packets, assume there are
     * more coming and immediately reschedule */
    if (ret >= n->tx_burst) {
        qemu_bh_schedule(q->tx_bh);
        q->tx_waiting = 1;
        return;
    }

    /* If less than a full burst, re-enable notification and flush
     * anything that may have come in while we weren't looking.  If
     * we find something, assume the guest is still active and reschedule */
    virtio_queue_set_notification(q->tx_vq, 1);
    ret = virtio_wifi_flush_tx(q);
    if (ret == -EINVAL) {
        return;
    } else if (ret > 0) {
        virtio_queue_set_notification(q->tx_vq, 0);
        qemu_bh_schedule(q->tx_bh);
        q->tx_waiting = 1;
    }
}

static void virtio_wifi_handle_rx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    qemu_flush_queued_packets(qemu_get_subqueue(n->nic, 0));
}

static void virtio_wifi_handle_ctrl(VirtIODevice* vdev, VirtQueue* vq) {}

static int fds[2];
/*set the sokcet for hostapd*/
extern void set_virtio_sock(int sock);

// packets from hostpad socket will always be mac80211 frames
void virtio_wifi_handle_frame(void* opaque, int fd, unsigned events) {
    VirtIOWifi* n = (VirtIOWifi*)opaque;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    // TODO (wdu@) Experiement with shared memory approach
    unsigned char buf[IEEE80211_MAX_FRAME_LEN];
    int size = socket_recv(fd, buf, sizeof(buf));
    if (size > 0) {
        rcu_read_lock();
        virtio_wifi_add_buf(vdev, n->vqs->rx_mgmt, buf, size);
        rcu_read_unlock();
    }
}

static void virtio_wifi_device_realize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);
    NetClientState* nc;

    if (socket_pair(&fds[0], &fds[1]) < 0) {
        fprintf(stderr, "%s: Failed to create socket pair! \n", __func__);
        return;
    }

    n->sock = fds[0];
    set_virtio_sock(fds[1]);
    n->loop_io = loopIo_new(looper_getForThread(), n->sock,
                            virtio_wifi_handle_frame, n);
    loopIo_wantRead(n->loop_io);
    loopIo_dontWantWrite(n->loop_io);

    // virtio_wifi_set_config_size(n, n->host_features);
    virtio_init(vdev, "virtio-wifi", VIRTIO_ID_MAC80211_WLAN, 64);
    n->vqs = g_malloc0(sizeof(VirtIOWifiQueue));
    // add queue
    int index = 0;
    n->tx_burst = TX_BURST;
    n->vqs[index].rx_data = virtio_add_queue(
            vdev, VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE, virtio_wifi_handle_rx);
    n->vqs[index].rx_mgmt =
            virtio_add_queue(vdev, VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE, NULL);
    n->vqs[index].tx_vq = virtio_add_queue(
            vdev, VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE, virtio_wifi_handle_tx);
    n->vqs[index].tx_bh = qemu_bh_new(virtio_wifi_tx_bh, &n->vqs[index]);
    n->vqs[index].tx_waiting = 0;
    n->vqs[index].n = n;
    n->ctrl_vq = virtio_add_queue(vdev, 64, virtio_wifi_handle_ctrl);

    // TODO(wdu@) Figure out a better way to set hard-coded mac address
    n->mac[0] = 0x02;
    n->mac[1] = 0x15;
    n->mac[2] = 0xb2;
    n->mac[3] = 0x00;
    n->mac[4] = 0x00;
    n->mac[5] = 0x00;

    n->status = VIRTIO_NET_S_LINK_UP;
    n->nic = qemu_new_nic(&wifi_virtio_info, &n->nic_conf,
                          object_get_typename(OBJECT(dev)), dev->id, n);
    /*peer_test_vnet_hdr(n);
    if (peer_has_vnet_hdr(n)) {
        for (i = 0; i < n->max_queues; i++) {
            qemu_using_vnet_hdr(qemu_get_subqueue(n->nic, i)->peer, true);
        }
        n->host_hdr_len = sizeof(struct virtio_net_hdr);
    } else {
        n->host_hdr_len = 0;
    }*/
    n->host_hdr_len = 0;

    n->qdev = dev;
}

static void virtio_wifi_device_unrealize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);
    int index = 0;
    virtio_del_queue(vdev, index * 2);
    virtio_del_queue(vdev, index * 2 + 1);
    loopIo_free(n->loop_io);
    close(n->sock);
    qemu_bh_delete(n->vqs->tx_bh);
    n->vqs->tx_bh = NULL;
    g_free(n->vqs);
    qemu_del_nic(n->nic);
    virtio_cleanup(vdev);
}

static void virtio_wifi_get_config(VirtIODevice* vdev, uint8_t* config) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    struct virtio_wifi_config cfg;
    memcpy(cfg.mac, n->mac, ETH_ALEN);
    memcpy(config, &cfg, sizeof(cfg));
}

static void virtio_wifi_set_config(VirtIODevice* vdev, const uint8_t* config) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    struct virtio_wifi_config cfg = {};
    memcpy(&cfg, config, sizeof(cfg));
    memcpy(n->mac, cfg.mac, ETH_ALEN);
}

static uint64_t virtio_wifi_get_features(VirtIODevice* vdev,
                                         uint64_t features,
                                         Error** errp) {
    return 0;
}

static void virtio_wifi_set_features(VirtIODevice* vdev, uint64_t features) {}

static uint64_t virtio_wifi_bad_features(VirtIODevice* vdev) {
    uint64_t features = 0;
    return features;
}

static void virtio_wifi_instance_init(Object* obj) {
    VirtIOWifi* n = VIRTIO_WIFI(obj);
}

static void virtio_wifi_reset(VirtIODevice* vdev) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    virtio_set_status(vdev, 0);
}

static void virtio_wifi_set_status(struct VirtIODevice* vdev, uint8_t status) {}

static Property virtio_wifi_properties[] = {
        DEFINE_NIC_PROPERTIES(VirtIOWifi, nic_conf),
        DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_virtio_wifi = {
    .name = "virtio-wifi-device",
    .minimum_version_id = 1,
    .version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_wifi_class_init(ObjectClass* klass, void* data) {
    DeviceClass* dc = DEVICE_CLASS(klass);
    VirtioDeviceClass* vdc = VIRTIO_DEVICE_CLASS(klass);

    dc->props = virtio_wifi_properties;
    dc->vmsd = &vmstate_virtio_wifi;
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    vdc->realize = virtio_wifi_device_realize;
    vdc->unrealize = virtio_wifi_device_unrealize;
    vdc->get_config = virtio_wifi_get_config;
    vdc->set_config = virtio_wifi_set_config;
    vdc->get_features = virtio_wifi_get_features;
    vdc->set_features = virtio_wifi_set_features;
    vdc->bad_features = virtio_wifi_bad_features;
    vdc->reset = virtio_wifi_reset;
    vdc->set_status = virtio_wifi_set_status;
    // vdc->guest_notifier_mask = virtio_net_guest_notifier_mask;
    // vdc->guest_notifier_pending = virtio_net_guest_notifier_pending;
}

void set_status(VirtIOWifi* n, int status) {
    n->status = status;
}

static const TypeInfo virtio_wifi_info = {
        .name = TYPE_VIRTIO_WIFI,
        .parent = TYPE_VIRTIO_DEVICE,
        .instance_size = sizeof(VirtIOWifi),
        .instance_init = virtio_wifi_instance_init,
        .class_init = virtio_wifi_class_init,
};

static void virtio_register_types(void) {
    type_register_static(&virtio_wifi_info);
}

type_init(virtio_register_types)
