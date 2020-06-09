#include "android-qemu2-glue/emulation/VirtioWifiForwarder.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"

extern "C" {
/* Because android::base::IOVector has its own definition of
 struct iovec, we need to avoid the re-definition of iovec
 from qemu/osdep.h by declaring CONFIG_IOVEC beforehand. */
#define CONFIG_IOVEC
#include "qemu/osdep.h"
#include "hw/virtio/virtio.h"
#include "android-qemu2-glue/emulation/virtio-wifi.h"
#include "standard-headers/linux/virtio_ids.h"
}

/*
 * Undefine type_check to avoid using typedef which is not supported.
 */
#undef type_check
#define type_check(t1, t2) 0

using android::base::IOVector;
using android::base::LazyInstance;
using android::network::Ieee80211Frame;
using android::qemu2::VirtioWifiForwarder;

namespace {
/* Limit the number of packets that can be sent via a single flush
 * of the TX queue.  This gives us a guaranteed exit condition and
 * ensures fairness in the io path.  256 conveniently matches the
 * length of the TX queue and shows a good balance of performance
 * and latency. */
static constexpr size_t kTXBurst = 256;

static constexpr size_t VIRTIO_WIFI_CTRL_QUEUE_DEFAULT_SIZE = 64;
static constexpr size_t VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE = 256;
static constexpr size_t VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE = 256;

static constexpr uint16_t VIRTIO_LINK_UP = 1;

static const uint8_t kBssID[] = {0x00, 0x13, 0x10, 0x85, 0xfe, 0x01};
static const uint8_t kMacAddr[] = {0x02, 0x15, 0xb2, 0x00, 0x00, 0x00};

static size_t virtio_wifi_on_frame_available(const uint8_t*, size_t, bool);

static void virtio_wifi_set_link_status(NetClientState* nc);

static int virtio_wifi_can_receive(NetClientState* nc);

static void virtio_wifi_tx_complete(NetClientState* nc, ssize_t len);

struct Globals {
    bool initWifiForwarder(NICConf* conf) {
        if (!wifiForwarder) {
            wifiForwarder.reset(new VirtioWifiForwarder(
                    bssID, &virtio_wifi_on_frame_available,
                    &virtio_wifi_set_link_status, &virtio_wifi_can_receive,
                    conf, &virtio_wifi_tx_complete));
            return wifiForwarder->init();
        } else {
            return true;
        }
    }

    const uint8_t* bssID = kBssID;
    int macPrefix = 0;
    std::unique_ptr<VirtioWifiForwarder> wifiForwarder = nullptr;
    VirtIOWifi* wifi = nullptr;
};

static LazyInstance<Globals> sGlobal = LAZY_INSTANCE_INIT;

struct virtio_wifi_config {
    uint8_t mac[ETH_ALEN];
    /* Default maximum transmit unit advice */
    uint16_t mtu;
} __attribute__((packed));

static bool virtio_wifi_has_buffers(VirtQueue* vq) {
    if (virtio_queue_empty(vq)) {
        virtio_queue_set_notification(vq, 1);

        /* To avoid a race condition where the guest has made some buffers
         * available after the above check but before notification was
         * enabled, check for available buffers again.
         */
        if (virtio_queue_empty(vq)) {
            return false;
        }
    }

    virtio_queue_set_notification(vq, 0);
    return true;
}

static ssize_t virtio_wifi_add_buf(VirtIODevice* vdev,
                                   VirtQueue* vq,
                                   const uint8_t* buf,
                                   size_t size) {
    if (!virtio_wifi_has_buffers(vq)) {
        LOG(VERBOSE) << "VirtIO WiFi: unexpected full virtqueue";
        return 0;
    }

    VirtQueueElement* elem = static_cast<VirtQueueElement*>(
            virtqueue_pop(vq, sizeof(VirtQueueElement)));
    if (!elem) {
        LOG(VERBOSE) << "VirtIO WiFi: unexpected empty virtqueue";
        return 0;
    }

    IOVector iovec;
    for (size_t i = 0; i < elem->in_num; i++) {
        iovec.push_back(elem->in_sg[i]);
    }
    size = std::min(size, Ieee80211Frame::MAX_FRAME_LEN);
    iovec.copyFrom(buf, 0, size);
    /* signal other side */
    virtqueue_fill(vq, elem, size, 0);
    // Use C free API because virtqueue_pop is using C alloc.
    g_free(elem);
    virtqueue_flush(vq, 1);
    virtio_notify(vdev, vq);
    return size;
}

static size_t virtio_wifi_on_frame_available(const uint8_t* buf,
                                             size_t size,
                                             bool fromRemoteVM) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    size_t ret = 0;
    if (size > 0) {
        rcu_read_lock();
        ret = virtio_wifi_add_buf(vdev, n->vqs->rx, buf, size);
        rcu_read_unlock();
    }
    return ret;
}

static int virtio_wifi_can_receive(NetClientState* nc) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);

    if (!vdev->vm_running) {
        return 0;
    }
    if (!virtio_queue_ready(n->vqs->rx) ||
        !(vdev->status & VIRTIO_CONFIG_S_DRIVER_OK)) {
        return 0;
    }
    return 1;
}

// If the vq is tx_p2p, then it is destined for remote VM.
static ssize_t virtio_wifi_flush_tx(VirtIOWifiQueue* q, VirtQueue* vq) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    size_t num_packets = 0;
    bool toRemote = (vq == n->vqs->tx_p2p);

    if (!toRemote && q->async_tx.elem) {
        virtio_queue_set_notification(vq, 0);
        return num_packets;
    }
    for (;;) {
        VirtQueueElement* elem = static_cast<VirtQueueElement*>(
                virtqueue_pop(vq, sizeof(VirtQueueElement)));
        if (!elem) {
            break;
        }
        if (elem->out_num < 1) {
            virtqueue_detach_element(vq, elem, 0);
            g_free(elem);
            return -EINVAL;
        }
        IOVector iovec;
        for (size_t i = 0; i < elem->out_num; i++) {
            iovec.push_back(elem->out_sg[i]);
        }
        if (sGlobal->wifiForwarder->forwardFrame(iovec, toRemote) == -EBUSY) {
            virtio_queue_set_notification(vq, 0);
            q->async_tx.elem = elem;
            return -EBUSY;
        }

        virtqueue_push(vq, elem, 0);
        virtio_notify(vdev, vq);
        // Use C free API because virtqueue_pop is using C alloc.
        g_free(elem);
        if (++num_packets >= n->tx_burst) {
            break;
        }
    }
    return num_packets;
}

// This function will only be called when the frame is sent to qemu networking.
// Thus, we can safely assume the virtqueue is always q->tx;
static void virtio_wifi_tx_complete(NetClientState* nc, ssize_t len) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIOWifiQueue* q = n->vqs;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    if (q->async_tx.elem) {
        virtqueue_push(q->tx, q->async_tx.elem, 0);
        virtio_notify(vdev, q->tx);
        g_free(q->async_tx.elem);
        q->async_tx.elem = NULL;
    }
    virtio_queue_set_notification(q->tx, 1);
    virtio_wifi_flush_tx(q, q->tx);
}

// set virtio-wifi link status according to netclientstate
static void virtio_wifi_set_link_status(NetClientState* nc) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    uint16_t old_status = n->status;
    if (nc->link_down)
        n->status &= ~VIRTIO_LINK_UP;
    else
        n->status |= VIRTIO_LINK_UP;

    if (n->status != old_status)
        virtio_notify_config(vdev);
}

}  // namespace

extern "C" {

static const VMStateDescription vmstate_virtio_wifi = {
        .name = "virtio-wifi-device",
        .version_id = 1,
        .minimum_version_id = 1,
        .fields =
                (VMStateField[]){VMSTATE_VIRTIO_DEVICE, VMSTATE_END_OF_LIST()},
};

void virtio_wifi_set_mac_prefix(int prefix) {
    if (sGlobal->wifi) {
        sGlobal->wifi->mac[4] = (prefix >> 8) & 0xff;
        sGlobal->wifi->mac[5] = prefix & 0xff;
    }
    sGlobal->macPrefix = prefix;
}

static QEMUBH* get_qemubh(VirtQueue* vq) {
    VirtIOWifiQueue* q = sGlobal->wifi->vqs;
    if (vq == q->tx) {
        return q->tx_bh;
    } else if (vq == q->tx_p2p) {
        return q->tx_p2p_bh;
    } else {
        return NULL;
    }
}

static void virtio_wifi_handle_tx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    VirtIOWifiQueue* q = n->vqs;
    if (unlikely(n->status & VIRTIO_LINK_UP) == 0) {
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
    QEMUBH* bh = get_qemubh(vq);
    qemu_bh_schedule(bh);
}

static void virtio_wifi_tx_bh(void* opaque) {
    VirtIOWifi* n = sGlobal->wifi;
    VirtIOWifiQueue* q = n->vqs;
    VirtQueue* vq = static_cast<VirtQueue*>(opaque);
    VirtIODevice* vdev = VIRTIO_DEVICE(n);

    /* This happens when device was stopped but BH wasn't. */
    if (!vdev->vm_running) {
        /* Make sure tx waiting is set, so we'll run when restarted. */
        assert(q->tx_waiting);
        return;
    }

    q->tx_waiting = 0;

    ssize_t ret = virtio_wifi_flush_tx(q, vq);
    if (ret == -EBUSY || ret == -EINVAL) {
        return; /* Notification re-enable handled by tx_complete or device
                 * broken */
    }
    QEMUBH* bh = get_qemubh(vq);
    /* If we flush a full burst of packets, assume there are
     * more coming and immediately reschedule */
    if (ret >= n->tx_burst) {
        qemu_bh_schedule(bh);
        q->tx_waiting = 1;
        return;
    }

    /* If less than a full burst, re-enable notification and flush
     * anything that may have come in while we weren't looking.  If
     * we find something, assume the guest is still active and reschedule */
    virtio_queue_set_notification(vq, 1);
    ret = virtio_wifi_flush_tx(q, vq);
    if (ret > 0) {
        virtio_queue_set_notification(vq, 0);
        qemu_bh_schedule(bh);
        q->tx_waiting = 1;
    }
}

static void virtio_wifi_handle_rx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    qemu_flush_queued_packets(
            qemu_get_subqueue(sGlobal->wifiForwarder->getNic(), 0));
}

static void virtio_wifi_handle_ctrl(VirtIODevice* vdev, VirtQueue* vq) {}

static void virtio_wifi_device_realize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);

    if (!sGlobal->initWifiForwarder(&n->nic_conf)) {
        LOG(WARNING) << "VirtIO WiFi device failed to initialize.";
        virtio_cleanup(vdev);
    }

    virtio_init(vdev, "virtio-wifi", VIRTIO_ID_MAC80211_WLAN, 64);
    n->vqs = new VirtIOWifiQueue;
    // add queue
    int index = 0;
    n->tx_burst = kTXBurst;
    n->vqs[index].rx = virtio_add_queue(vdev, VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE,
                                        virtio_wifi_handle_rx);
    n->vqs[index].tx = virtio_add_queue(vdev, VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE,
                                        virtio_wifi_handle_tx);
    n->ctrl_vq = virtio_add_queue(vdev, 64, virtio_wifi_handle_ctrl);
    n->vqs[index].tx_p2p = virtio_add_queue(
            vdev, VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE, virtio_wifi_handle_tx);

    n->vqs[index].tx_bh = qemu_bh_new(virtio_wifi_tx_bh, n->vqs[index].tx);
    n->vqs[index].tx_p2p_bh =
            qemu_bh_new(virtio_wifi_tx_bh, n->vqs[index].tx_p2p);

    n->vqs[index].tx_waiting = 0;
    n->vqs[index].n = n;
    memcpy(n->mac, kMacAddr, ETH_ALEN);
    n->mac[4] = (sGlobal->macPrefix >> 8) & 0xff;
    n->mac[5] = sGlobal->macPrefix & 0xff;
    n->status = VIRTIO_LINK_UP;
    n->host_hdr_len = 0;
    n->qdev = dev;
}

static void virtio_wifi_device_unrealize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);
    virtio_del_queue(vdev, 0);
    virtio_del_queue(vdev, 1);
    virtio_del_queue(vdev, 2);
    virtio_del_queue(vdev, 3);
    qemu_bh_delete(n->vqs->tx_bh);
    n->vqs->tx_bh = NULL;
    qemu_bh_delete(n->vqs->tx_p2p_bh);
    n->vqs->tx_p2p_bh = NULL;
    delete n->vqs;
    sGlobal->wifiForwarder->stop();
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
    // Keep reference to VirtIOWifi so that callback functions
    // register to the VirtioWifiForwarder will not need to pass
    // an opaque pointer which is essentially VirtIOWifi ptr.
    sGlobal->wifi = n;
}

static void virtio_wifi_reset(VirtIODevice* vdev) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
}

static void virtio_wifi_set_status(struct VirtIODevice* vdev, uint8_t status) {}

static Property virtio_wifi_properties[] = {
        DEFINE_NIC_PROPERTIES(VirtIOWifi, nic_conf),
        DEFINE_PROP_END_OF_LIST(),
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

}  // extern "C"
