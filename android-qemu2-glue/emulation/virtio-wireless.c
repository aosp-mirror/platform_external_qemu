#include "qemu/osdep.h"

#include "qemu/iov.h"
#include "hw/virtio/virtio.h"
#include "net/net.h"
#include "net/ieee80211_common_defs.h"
#include "hw/virtio/virtio-pci.h"
#include "android/utils/sockets.h"
#include "android/utils/looper.h"
#include "slirp/slirp.h"

#include "standard-headers/linux/if_ether.h"
#include "android-qemu2-glue/emulation/virtio-wireless.h"
//#include "standard-headers/linux/virtio_net.h"


#define TX_TIMER_INTERVAL 150000 /* 150 us */

/* Limit the number of packets that can be sent via a single flush
 * of the TX queue.  This gives us a guaranteed exit condition and
 * ensures fairness in the io path.  256 conveniently matches the
 * length of the TX queue and shows a good balance of performance
 * and latency. */
#define TX_BURST 256

/* Maximum packet size we can receive from tap device: header + 64k */
//#define VIRTIO_NET_MAX_BUFSIZE (sizeof(struct virtio_net_hdr) + (64 << 10))

/* previously fixed value */
#define VIRTIO_WIRELESS_RX_QUEUE_DEFAULT_SIZE 256
#define VIRTIO_WIRELESS_TX_QUEUE_DEFAULT_SIZE 256

/* for now, only allow larger queues; with virtio-1, guest can downsize */
#define VIRTIO_WIRELESS_RX_QUEUE_MIN_SIZE VIRTIO_WIRELESS_RX_QUEUE_DEFAULT_SIZE
#define VIRTIO_WIRELESS_TX_QUEUE_MIN_SIZE VIRTIO_WIRELESS_TX_QUEUE_DEFAULT_SIZE

static int vq2q(int queue_index) {
    return queue_index / 2;
}

static void peer_test_vnet_hdr(VirtIOWireless *n)
{
    NetClientState *nc = qemu_get_queue(n->nic);
    if (!nc->peer) {
        return;
    }

    n->has_vnet_hdr = qemu_has_vnet_hdr(nc->peer);
}

static int virtio_wireless_can_receive(NetClientState *nc) {
	return 1;
}

// set virtio-wireless link status according to netclientstate
static void virtio_wireless_set_link_status(NetClientState *nc) {

}


// TODO (wdu@) Figure out what is the best way to determine if we can write
// to the virtqueue.
static int virtio_wireless_has_buffers(VirtQueue * vq, int bufsize)
{
    /*VirtIOWireless *n = q->n;
    if (virtio_queue_empty(q->rx_vq) ||
        (n->mergeable_rx_bufs &&
         !virtqueue_avail_bytes(q->rx_vq, bufsize, 0))) {
        virtio_queue_set_notification(q->rx_vq, 1);*/

        /* To avoid a race condition where the guest has made some buffers
         * available after the above check but before notification was
         * enabled, check for available buffers again.
         */
        /*if (virtio_queue_empty(q->rx_vq) ||
            (n->mergeable_rx_bufs &&
             !virtqueue_avail_bytes(q->rx_vq, bufsize, 0))) {
            return 0;
        }
    }*/

    virtio_queue_set_notification(vq, 0);
    return 1;
}

static ssize_t virtio_wireless_add_buf(VirtIODevice *vdev, VirtQueue * vq, const uint8_t *buf, size_t size) {
    VirtQueueElement *elem;
    struct iovec *sg;
    size_t guest_offset, offset;
    offset = guest_offset = 0;
    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    if (!elem) {
        virtio_error(vdev, "virtio-wireless unexpected empty queue");
        return -1;
    }
    sg = elem->in_sg;
    size_t len = iov_from_buf(sg, elem->in_num, guest_offset,
                       buf + offset, size - offset);

    /* signal other side */
    virtqueue_fill(vq, elem, len, 0);
    virtqueue_flush(vq, 1);
    virtio_notify(vdev, vq);
    g_free(elem);
    return size;
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
const unsigned char rfc1042_header[] =
    { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
const unsigned char bridge_tunnel_header[] =
    { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

static int ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
    const u16 *a = (const u16 *)addr1;
    const u16 *b = (const u16 *)addr2;

    return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}

static uint8_t* ieee80211_data_from_8023(const uint8_t* buf, const u8* da_addr,
                 const u8 *bssid, int qos, size_t* psize) {
    struct ieee80211_hdr hdr;
    u16 hdrlen, ethertype;
    le16 fc;
    const u8 *encaps_data;
    int encaps_len, skip_header_bytes;
    if (*psize < 8) {
        return NULL;
    }
    ethertype = (buf[12] << 8) | buf[13];
    if (qos) {
        fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_QOS_DATA);
    } else {
        fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    }
    fc |= WLAN_FC_FROMDS;
    /* DA BSSID SA */

    memcpy(hdr.addr1, da_addr, ETH_ALEN);
    memcpy(hdr.addr2, bssid, ETH_ALEN);
    memcpy(hdr.addr3, buf + ETH_ALEN, ETH_ALEN);
    hdrlen = IEEE80211_HDRLEN + (qos ? 2 : 0);
    hdr.frame_control = fc;
    hdr.duration_id = 0;
    hdr.seq_ctrl = 0;
    skip_header_bytes = ETH_HLEN;
    if (ethertype >= 0x0600) {
        encaps_data = rfc1042_header;
        encaps_len = sizeof(rfc1042_header);
        skip_header_bytes -= 2;
    } else {
        encaps_data = NULL;
        encaps_len = 0;
    }
    size_t orig_size = *psize;
    *psize = *psize - skip_header_bytes + hdrlen + encaps_len;
    uint8_t* data = malloc(*psize + 1);
    memcpy(data, &hdr, hdrlen); 
    if (encaps_len) {
        memcpy(data + hdrlen, encaps_data, encaps_len);
    }

    memcpy(data + (hdrlen + encaps_len), buf + skip_header_bytes, orig_size - skip_header_bytes);

    return data;
}

// Assuming ethernet frames from VNIC is always data frames so we can safely 
// forward them back to the guest.
static ssize_t virtio_wireless_receive_rcu(NetClientState *nc, const uint8_t *buf,
                                      size_t size)
{
    VirtIOWireless *n = qemu_get_nic_opaque(nc);
    VirtIOWirelessQueue *q = &n->vqs[0];
    VirtIODevice *vdev = VIRTIO_DEVICE(n);

    if (!virtio_wireless_can_receive(nc)) {
        return -1;
    }

    /* hdr_len refers to the header we supply to the guest */
    if (!virtio_wireless_has_buffers(q->rx_vq, size /*+ n->guest_hdr_len - n->host_hdr_len*/)) {
        return 0;
    }

    int qos = 1;
    u8 da_addr[ETH_ALEN];
    da_addr[0] = 0x02;
    da_addr[1] = (4088 >> 8) & 0xFF;
    da_addr[2] = 4088 & 0xFF;
    da_addr[3] = 1 >> 8;
    da_addr[4] = 1;
    da_addr[5] = 0;
    const u8 bssid[] = {0x00, 0x13, 0x10, 0x85, 0xfe, 0x01};
    uint8_t* data = ieee80211_data_from_8023(buf, da_addr,
                 bssid, qos , &size);

    ssize_t s = virtio_wireless_add_buf(vdev, q->rx_vq, data, size);
    free(data);
    return s;
}


static ssize_t virtio_wireless_receive(NetClientState *nc, const uint8_t *buf,
                                  size_t size) {

    ssize_t r;
    rcu_read_lock();
    r = virtio_wireless_receive_rcu(nc, buf, size);
    rcu_read_unlock();
    return r;
}

static NetClientInfo wireless_virtio_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .can_receive = virtio_wireless_can_receive,
    .receive = virtio_wireless_receive,
    .link_status_changed = virtio_wireless_set_link_status,
    //.query_rx_filter = virtio_net_query_rxfilter,
};

// Assume the mac80211 is toDS
static uint8_t* ieee80211_data_to_8023(uint8_t* buf, size_t* psize, int qos) {
    u16 hdrlen = IEEE80211_HDRLEN + sizeof(rfc1042_header) + (qos ? 2 : 0);
    u16 ethertype;
    u8 *payload;
    u8 dst[ETH_ALEN];
    u8 src[ETH_ALEN];

    if (*psize < hdrlen + 8) {
        return NULL;
    }
    ethertype = (buf[hdrlen] << 8) | buf[hdrlen + 1];
    payload = buf + (hdrlen - ETH_ALEN * 2);

    struct ieee8023_hdr* eth_hdr = (struct ieee8023_hdr*) payload;
    if (ethertype == ETH_P_IPV6) {
        eth_hdr->ethertype = htons(ETH_P_IPV6);
    } else if (ethertype == ETH_P_IP) {
        eth_hdr->ethertype = htons(ETH_P_IP);
    } else if (ethertype == ETH_P_ARP) {
        eth_hdr->ethertype = htons(ETH_P_ARP);
    }
    *psize -= (hdrlen - ETH_ALEN * 2);
    return payload;
}

static int32_t virtio_wireless_flush_tx(VirtIOWirelessQueue *q)
{
    VirtIOWireless *n = q->n;
    VirtIODevice *vdev = VIRTIO_DEVICE(n);
    VirtQueueElement *elem;
    int32_t num_packets = 0;
    int queue_index = vq2q(virtio_get_queue_index(q->tx_vq));
    uint8_t buf[4096];
    for (;;) {
        ssize_t ret;
        unsigned int out_num;
        struct iovec*out_sg;
        elem = virtqueue_pop(q->tx_vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }

        out_num = elem->out_num;
        out_sg = elem->out_sg;
        if (out_num < 1) {
            virtio_error(vdev, "virtio-wireless header not in first element");
            virtqueue_detach_element(q->tx_vq, elem, 0);
            g_free(elem);
            return -EINVAL;
        }

        size_t size = iov_to_buf(out_sg, out_num, 0, buf, 4096);
        if (size >= 10) {
            if (is_frame_tods(buf)) {
                int qos = ieee8021_is_data_qos(buf);
                uint8_t* ether_buf = ieee80211_data_to_8023(buf, &size, qos);
                if (ether_buf) {
                    qemu_send_packet(qemu_get_subqueue(n->nic, 0), ether_buf, size);
                }
            } else {
                send(n->sock, buf, size, 0);
            }
        }
    }
    return num_packets;
}

static void virtio_wireless_handle_tx(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(vdev);
    VirtIOWirelessQueue *q = &n->vqs[0];
    if (unlikely(q->tx_waiting)) {
        return;
    }
    q->tx_waiting = 1;
    /* This happens when device was stopped but VCPU wasn't. */
    if (!vdev->vm_running) {
        return;
    }
    virtio_queue_set_notification(vq, 0);
    qemu_bh_schedule(q->tx_bh);
}

static void virtio_wireless_tx_bh(void *opaque)
{
    VirtIOWirelessQueue *q = opaque;
    VirtIOWireless *n = q->n;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    int32_t ret;

    /* This happens when device was stopped but BH wasn't. */
    if (!vdev->vm_running) {
        /* Make sure tx waiting is set, so we'll run when restarted. */
        assert(q->tx_waiting);
        return;
    }

    q->tx_waiting = 0;

    ret = virtio_wireless_flush_tx(q);
    if (ret == -EBUSY || ret == -EINVAL) {
        fprintf(stderr, "virtio_wireless_flush_tx returns EBUSY or EINVAL \n");
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
    ret = virtio_wireless_flush_tx(q);
    if (ret == -EINVAL) {
        return;
    } else if (ret > 0) {
        virtio_queue_set_notification(q->tx_vq, 0);
        qemu_bh_schedule(q->tx_bh);
        q->tx_waiting = 1;
    }
}

static void virtio_wireless_handle_rx(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(vdev);
    int queue_index = vq2q(virtio_get_queue_index(vq));
}

static void virtio_wireless_handle_ctrl(VirtIODevice *vdev, VirtQueue *vq)
{

}

static int fds[2];
/*set the sokcet for hostapd*/
extern void set_virtio_sock(int sock);

//packets from hostpad socket will always be mac80211 frames
void virtio_handle_frame(void* opaque, int fd, unsigned events) {
    VirtIOWireless *n = (VirtIOWireless *)opaque;
    VirtIODevice *vdev = VIRTIO_DEVICE(n);
    unsigned char buf[4096];
    assert(fd == n->sock);
    assert((events & LOOP_IO_READ) != 0);
    int size = socket_recv(fd, buf, sizeof(buf));
    if (size > 0) {
        virtio_wireless_add_buf(vdev, n->vqs->rx_vq, buf, size);
    }
}   

static void virtio_wireless_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOWireless *n = VIRTIO_WIRELESS(dev);
    NetClientState *nc;

    if (socket_pair(&fds[0], &fds[1]) < 0) {
        fprintf(stderr, "%s: Failed to create socket pair! \n", __func__);
        return;
    }
    n->sock = fds[0];
    set_virtio_sock(fds[1]);

    n->loop_io = loopIo_new(looper_getForThread(), n->sock, virtio_handle_frame, n);
    loopIo_wantRead(n->loop_io);
    loopIo_dontWantWrite(n->loop_io);
    
    //virtio_wireless_set_config_size(n, n->host_features);
    virtio_init(vdev, "virtio-wireless", VIRTIO_ID_WIRELESS, 64);
	n->vqs = g_malloc0(sizeof(VirtIOWirelessQueue));
	// add queue
	int index = 0;
    n->tx_burst = TX_BURST;
	n->vqs[index].rx_vq = virtio_add_queue(vdev, VIRTIO_WIRELESS_RX_QUEUE_DEFAULT_SIZE,
                                           virtio_wireless_handle_rx);
    
    n->vqs[index].tx_vq =
        virtio_add_queue(vdev, VIRTIO_WIRELESS_TX_QUEUE_DEFAULT_SIZE,
                         virtio_wireless_handle_tx);
    n->vqs[index].tx_bh = qemu_bh_new(virtio_wireless_tx_bh, &n->vqs[index]);
    n->vqs[index].tx_waiting = 0;
    n->vqs[index].n = n;
	n->ctrl_vq = virtio_add_queue(vdev, 64, virtio_wireless_handle_ctrl);
	qemu_macaddr_default_if_unset(&n->nic_conf.macaddr);
	memcpy(&n->mac[0], &n->nic_conf.macaddr, sizeof(n->mac));

	n->status = VIRTIO_NET_S_LINK_UP;
	n->nic = qemu_new_nic(&wireless_virtio_info, &n->nic_conf,
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

static void virtio_wireless_device_unrealize(DeviceState *dev, Error **errp)
{

    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOWireless *n = VIRTIO_WIRELESS(dev);
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

static void virtio_wireless_get_config(VirtIODevice *vdev, uint8_t *config) {

}

static void virtio_wireless_set_config(VirtIODevice *vdev, const uint8_t *config)
{

}

static uint64_t virtio_wireless_get_features(VirtIODevice *vdev, uint64_t features,
                                        Error **errp)
{
    return 0;
}

static void virtio_wireless_set_features(VirtIODevice *vdev, uint64_t features)
{

}

static uint64_t virtio_wireless_bad_features(VirtIODevice *vdev)
{
    uint64_t features = 0;
    return features;
}

static void virtio_wireless_instance_init(Object *obj)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(obj);
}

static void virtio_wireless_reset(VirtIODevice *vdev)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(vdev);
}

static void virtio_wireless_set_status(struct VirtIODevice *vdev, uint8_t status)
{

}

static Property virtio_wireless_properties[] = {
    DEFINE_NIC_PROPERTIES(VirtIOWireless, nic_conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_wireless_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    dc->props = virtio_wireless_properties;
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    vdc->realize = virtio_wireless_device_realize;
    vdc->unrealize = virtio_wireless_device_unrealize;
    vdc->get_config = virtio_wireless_get_config;
    vdc->set_config = virtio_wireless_set_config;
    vdc->get_features = virtio_wireless_get_features;
    vdc->set_features = virtio_wireless_set_features;
    vdc->bad_features = virtio_wireless_bad_features;
    vdc->reset = virtio_wireless_reset;
    vdc->set_status = virtio_wireless_set_status;
    //vdc->guest_notifier_mask = virtio_net_guest_notifier_mask;
    //vdc->guest_notifier_pending = virtio_net_guest_notifier_pending;
}
 
void set_status(VirtIOWireless* n, int status) {
    n->status = status;
}

static const TypeInfo virtio_wireless_info = {
    .name = TYPE_VIRTIO_WIRELESS,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOWireless),
    .instance_init = virtio_wireless_instance_init,
    .class_init = virtio_wireless_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_wireless_info);
}

type_init(virtio_register_types)
