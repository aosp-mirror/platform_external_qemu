

#include "qemu/osdep.h"

#include "qemu/iov.h"
#include "hw/virtio/virtio.h"
#include "net/net.h"
#include "hw/virtio/virtio-pci.h"
#include "android/utils/sockets.h"
#include "android/utils/looper.h"

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


// Assuming ethernet frames from VNIC is always data frames so we can safely 
// forward them back to the guest.
static ssize_t virtio_wireless_receive_rcu(NetClientState *nc, const uint8_t *buf,
                                      size_t size)
{
    VirtIOWireless *n = qemu_get_nic_opaque(nc);
    VirtIOWirelessQueue *q = &n->vqs[nc->queue_index];
    VirtIODevice *vdev = VIRTIO_DEVICE(n);

    if (!virtio_wireless_can_receive(nc)) {
        return -1;
    }

    /* hdr_len refers to the header we supply to the guest */
    if (!virtio_wireless_has_buffers(q->rx_vq, size /*+ n->guest_hdr_len - n->host_hdr_len*/)) {
        return 0;
    }

    // TODO (wdu@) missing steps
    // 1, Determine guest header length and host header length for the packets.
    // 2, Convert ethernet frame to wireless frame (is this achievable? or do we need to rely on hostapd to generate this for us?)
    return virtio_wireless_add_buf(vdev, q->rx_vq, buf, size);
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

static int32_t virtio_wireless_flush_tx(VirtIOWirelessQueue *q)
{
    VirtIOWireless *n = q->n;
    VirtIODevice *vdev = VIRTIO_DEVICE(n);
    VirtQueueElement *elem;
    int32_t num_packets = 0;
    int queue_index = vq2q(virtio_get_queue_index(q->tx_vq));
    //if (!(vdev->status & VIRTIO_CONFIG_S_DRIVER_OK)) {
    //    return num_packets;
    //}

    /*if (q->async_tx.elem) {
        virtio_queue_set_notification(q->tx_vq, 0);
        return num_packets;
    }*/
    uint8_t buf[4096];
    for (;;) {
        ssize_t ret;
        unsigned int out_num;
        struct iovec sg[VIRTQUEUE_MAX_SIZE], sg2[VIRTQUEUE_MAX_SIZE + 1], *out_sg;
        struct virtio_net_hdr_mrg_rxbuf mhdr;

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


        /*
         * If host wants to see the guest header as is, we can
         * pass it on unchanged. Otherwise, copy just the parts
         * that host is interested in.
         */
        assert(n->host_hdr_len <= n->guest_hdr_len);

        size_t size = iov_to_buf(out_sg, out_num, 0, buf, 4096);

        send(n->sock, buf, size, 0);

        /*ret = qemu_sendv_packet_async(qemu_get_subqueue(n->nic, queue_index),
                                      out_sg, out_num, virtio_net_tx_complete);*/
        /*if (ret == 0) {
            virtio_queue_set_notification(q->tx_vq, 0);
            q->async_tx.elem = elem;
            return -EBUSY;
        }*/
    }
    return num_packets;
}

static void virtio_wireless_handle_tx(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(vdev);
    int queue_index = vq2q(virtio_get_queue_index(vq));
    // send the packet to sock
    //virtio_queue_set_notification(vq, 0);
    virtio_wireless_flush_tx(n->vqs);

}

static void virtio_wireless_handle_rx(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOWireless *n = VIRTIO_WIRELESS(vdev);
    int queue_index = vq2q(virtio_get_queue_index(vq));
    /*VirtQueueElement *elem;
    size_t s;
    struct iovec *iov;
    unsigned int iov_cnt;
    for (;;) {
        elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        iov_cnt = elem->out_num;
        iov = g_memdup(elem->out_sg, sizeof(struct iovec) * elem->out_num);

    }*/

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
        fprintf(stderr, "%s: received %d bytes \n", __func__, size);
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
	n->vqs[index].rx_vq = virtio_add_queue(vdev, VIRTIO_WIRELESS_RX_QUEUE_DEFAULT_SIZE,
                                           virtio_wireless_handle_rx);
    
    n->vqs[index].tx_vq =
        virtio_add_queue(vdev, VIRTIO_WIRELESS_TX_QUEUE_DEFAULT_SIZE,
                         virtio_wireless_handle_tx);
    //n->vqs[index].tx_bh = qemu_bh_new(virtio_net_tx_bh, &n->vqs[index]);
    //}

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
    g_free(n->vqs);
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

static void virtio_wireless_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    //dc->props = virtio_net_properties;
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
