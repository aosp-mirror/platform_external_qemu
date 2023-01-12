#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "aemu/base/Log.h"
#include "android/telephony/sysdeps.h"
#include "host-common/VmLock.h"
#include "android-qemu2-glue/emulation/WifiService.h"
#include "android-qemu2-glue/utils/stream.h"
#include "android/cmdline-definitions.h"  // for AndroidOptions
#include "android/console.h"
#include "android/network/Ieee80211Frame.h"
#include "android/telephony/sysdeps.h"
#include "android/utils/debug.h"

extern "C" {
#include "android-qemu2-glue/emulation/virtio-wifi.h"
#include "migration/register.h"
#include "standard-headers/linux/virtio_ids.h"
}

#include <algorithm>
#include <cstring>

/*
 * Undefine type_check to avoid using typedef which is not supported.
 */
#undef type_check
#define type_check(t1, t2) 0

using android::base::IOVector;
using android::network::Ieee80211Frame;
using android::qemu2::WifiService;
using android::qemu2::HostapdOptions;
namespace {

/* Limit the number of packets that can be sent via a single flush
 * of the TX queue.  This gives us a guaranteed exit condition and
 * ensures fairness in the io path.  256 conveniently matches the
 * length of the TX queue and shows a good balance of performance
 * and latency. */
static constexpr size_t kTXBurst = 256;
static constexpr size_t kQueueSize = 1;

static constexpr size_t VIRTIO_WIFI_CTRL_QUEUE_DEFAULT_SIZE = 64;
static constexpr size_t VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE = 256;
static constexpr size_t VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE = 256;

constexpr uint16_t VIRTIO_LINK_UP = 1;

static const uint8_t kBssID[] = {0x00, 0x13, 0x10, 0x85, 0xfe, 0x01};
static const uint8_t kMacAddr[] = {0x02, 0x15, 0xb2, 0x00, 0x00, 0x00};
static const char kNicModel[] = "virtio-wifi-device";
static const char kNicName[] = "virtio-wifi-device";
static ssize_t virtio_wifi_on_frame_available(const uint8_t*, size_t);

static void virtio_wifi_set_link_status(NetClientState* nc);

static int virtio_wifi_can_receive(size_t queue_index);

static void virtio_wifi_tx_complete(size_t queue_index);

struct GlobalState {
    bool initWifiService(NICConf* conf) {
        if (!wifiService) {
            auto opts = getConsoleAgents()->settings->android_cmdLineOptions();
            HostapdOptions hostapd = {.disabled = true};
            auto builder =
                    WifiService::Builder()
                            .withRedirectToNetsim(opts->redirect_to_netsim)
                            .withHostapd(hostapd)
                            .withBssid(std::vector<uint8_t>(
                                    kBssID, kBssID + sizeof(kBssID)))
                            .withNicConf(conf)
                            .withVerboseLogging(VERBOSE_CHECK(wifi))
                            .withOnReceiveCallback(
                                    &virtio_wifi_on_frame_available)
                            .withOnSentCallback(&virtio_wifi_tx_complete)
                            .withCanReceiveCallback(&virtio_wifi_can_receive)
                            .withOnLinkStatusChangedCallback(
                                    &virtio_wifi_set_link_status);

            if (opts->wifi_client_port) {
                builder.withClientPort(std::stoi(opts->wifi_client_port));
            }
            if (opts->wifi_server_port) {
                builder.withServerPort(std::stoi(opts->wifi_server_port));
            }
            wifiService.reset(builder.build().release());
            return wifiService->init();
        } else {
            return true;
        }
    }
    int macPrefix = 0;
    std::unique_ptr<WifiService> wifiService = nullptr;
    VirtIOWifi* wifi = nullptr;
};

static GlobalState* globalState() {
    static GlobalState sGlobal;
    return &sGlobal;
}

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
                                   uint32_t size) {
    android::RecursiveScopedVmLock lock;
    if (!virtio_wifi_has_buffers(vq)) {
        LOG(VERBOSE) << "VirtIO WiFi: unexpected full virtqueue";
        return -1;
    }
    VirtQueueElement* elem = static_cast<VirtQueueElement*>(
            virtqueue_pop(vq, sizeof(VirtQueueElement)));
    if (!elem) {
        LOG(VERBOSE) << "VirtIO WiFi: unexpected empty virtqueue";
        return -1;
    }
    if (elem->in_num < 1) {
        virtqueue_detach_element(vq, elem, 0);
        g_free(elem);
        return -1;
    }
    IOVector iovec(elem->in_sg, elem->in_sg + elem->in_num);
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

static ssize_t virtio_wifi_on_frame_available(const uint8_t* buf, size_t size) {
    VirtIOWifi* n = globalState()->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    ssize_t ret = 0;
    if (size > 0) {
        NetClientState* nc =
                qemu_get_queue(globalState()->wifiService->getNic());
        size_t index = nc ? nc->queue_index : 0;
        ret = virtio_wifi_add_buf(vdev, n->vqs[index].rx, buf, size);
    }
    return ret;
}

static int virtio_wifi_can_receive(size_t queue_index) {
    VirtIOWifi* n = globalState()->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    if (!vdev->vm_running) {
        return 0;
    }
    if (!virtio_queue_ready(n->vqs[queue_index].rx) ||
        !(vdev->status & VIRTIO_CONFIG_S_DRIVER_OK)) {
        return 0;
    }
    return 1;
}

// If the vq is tx_p2p, then it is destined for remote VM.
static ssize_t virtio_wifi_flush_tx(VirtIOWifiQueue* q) {
    GlobalState& state = *globalState();
    VirtIOWifi* n = state.wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    VirtQueue* vq = q->tx;
    size_t num_packets = 0;

    if (q->async_tx.elem) {
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

        IOVector iovec(elem->out_sg, elem->out_sg + elem->out_num);
        if (state.wifiService->send(iovec) == -EBUSY) {
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
static void virtio_wifi_tx_complete(size_t queue_index) {
    VirtIOWifi* n = globalState()->wifi;
    VirtIOWifiQueue* q = &n->vqs[queue_index];
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    if (q->async_tx.elem) {
        virtqueue_push(q->tx, q->async_tx.elem, 0);
        virtio_notify(vdev, q->tx);
        g_free(q->async_tx.elem);
        q->async_tx.elem = NULL;
    }
    virtio_queue_set_notification(q->tx, 1);
    virtio_wifi_flush_tx(q);
}

// set virtio-wifi link status according to netclientstate
static void virtio_wifi_set_link_status(NetClientState* nc) {
    VirtIOWifi* n = globalState()->wifi;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);
    uint16_t old_status = n->status;
    if (nc->link_down)
        n->status &= ~VIRTIO_LINK_UP;
    else
        n->status |= VIRTIO_LINK_UP;

    if (n->status != old_status)
        virtio_notify_config(vdev);
}

static VirtIOWifiQueue* getWifiQueue(VirtQueue* vq) {
    VirtIOWifi* n = globalState()->wifi;
    return std::find_if(n->vqs, n->vqs + kQueueSize,
                        [vq](const VirtIOWifiQueue& q) { return q.tx == vq; });
}

}  // namespace

extern "C" {

static void virtio_wifi_state_save(QEMUFile* file, void* opaque) {
    Stream* const s = stream_from_qemufile(file);
    VirtIOWifi* n = static_cast<struct GlobalState*>(opaque)->wifi;
    sys_file_put_buffer((SysFile*)s, (uint8_t*)n->mac, ETH_ALEN);
    sys_file_put_be32((SysFile*)s, n->status);
    stream_free(s);
}

static int virtio_wifi_state_load(QEMUFile* file,
                                  void* opaque,
                                  int version_id) {
    Stream* const s = stream_from_qemufile(file);
    VirtIOWifi* n = static_cast<struct GlobalState*>(opaque)->wifi;
    sys_file_get_buffer((SysFile*)s, n->mac, ETH_ALEN);
    uint32_t status = sys_file_get_be32((SysFile*)s);
    n->status = status;
    stream_free(s);
    return 0;
}

static SaveVMHandlers virtio_vmhandlers = {
        .save_state = virtio_wifi_state_save,
        .load_state = virtio_wifi_state_load,
};

static const VMStateDescription vmstate_virtio_wifi = {
        .name = "virtio-wifi-device",
        .version_id = 1,
        .minimum_version_id = 1,
        .fields =
                (VMStateField[]){VMSTATE_VIRTIO_DEVICE, VMSTATE_END_OF_LIST()},
};

void virtio_wifi_set_mac_prefix(int prefix) {
    GlobalState& state = *globalState();
    if (state.wifi) {
        state.wifi->mac[4] = (prefix >> 8) & 0xff;
        state.wifi->mac[5] = prefix & 0xff;
    }
    state.macPrefix = prefix;
}

const uint8_t* virtio_wifi_ssid_to_ethaddr(const char* ssid) {
    GlobalState& state = *globalState();
    if (state.wifi) {
        auto mac = state.wifiService->getStaMacAddr(ssid);
        if (!mac.empty()) {
            std::memcpy(state.wifi->mac, mac.mAddr, ETH_ALEN);
            return reinterpret_cast<const uint8_t*>(state.wifi->mac);
        }
    }
    return nullptr;
}

static void virtio_wifi_handle_tx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    VirtIOWifiQueue* q = getWifiQueue(vq);
    if (unlikely(q == NULL)) {
        return;
    }
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
    qemu_bh_schedule(q->tx_bh);
}

static void virtio_wifi_tx_bh(void* opaque) {
    VirtIOWifi* n = globalState()->wifi;
    VirtIOWifiQueue* q = static_cast<VirtIOWifiQueue*>(opaque);
    VirtQueue* vq = q->tx;
    VirtIODevice* vdev = VIRTIO_DEVICE(n);

    /* This happens when device was stopped but BH wasn't. */
    if (!vdev->vm_running) {
        /* Make sure tx waiting is set, so we'll run when restarted. */
        assert(q->tx_waiting);
        return;
    }

    q->tx_waiting = 0;
    ssize_t ret = virtio_wifi_flush_tx(q);
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
    virtio_queue_set_notification(vq, 1);
    ret = virtio_wifi_flush_tx(q);
    if (ret > 0) {
        virtio_queue_set_notification(vq, 0);
        qemu_bh_schedule(q->tx_bh);
        q->tx_waiting = 1;
    }
}

static void virtio_wifi_handle_rx(VirtIODevice* vdev, VirtQueue* vq) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    NICState* nic = globalState()->wifiService->getNic();
    if (nic) {
        qemu_flush_queued_packets(qemu_get_subqueue(nic, 0));
    }
}

static void virtio_wifi_handle_ctrl(VirtIODevice* vdev, VirtQueue* vq) {}

static void virtio_wifi_device_realize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);
    GlobalState& state = *globalState();
    n->nic_conf.peers.queues = kQueueSize;

    if (!state.initWifiService(&n->nic_conf)) {
        LOG(WARNING) << "Mac80211 Hwsim device failed to initialize.";
        virtio_cleanup(vdev);
        return;
    }

    virtio_init(vdev, "virtio-wifi", VIRTIO_ID_MAC80211_HWSIM, 0);
    n->vqs = new VirtIOWifiQueue[kQueueSize];
    for (size_t index = 0; index < kQueueSize; index++) {
        n->vqs[index].tx = virtio_add_queue(
                vdev, VIRTIO_WIFI_TX_QUEUE_DEFAULT_SIZE, virtio_wifi_handle_tx);
        n->vqs[index].tx_bh = qemu_bh_new(virtio_wifi_tx_bh, &n->vqs[index]);
        n->vqs[index].rx = virtio_add_queue(
                vdev, VIRTIO_WIFI_RX_QUEUE_DEFAULT_SIZE, virtio_wifi_handle_rx);
        n->vqs[index].tx_waiting = 0;
        n->vqs[index].async_tx.elem = nullptr;
    }
    n->tx_burst = kTXBurst;

    memcpy(n->mac, kMacAddr, ETH_ALEN);
    n->mac[4] = (state.macPrefix >> 8) & 0xff;
    n->mac[5] = state.macPrefix & 0xff;
    n->status = VIRTIO_LINK_UP;
    n->qdev = dev;
    register_savevm_live(NULL, "virtio_wifi", 0, 0, &virtio_vmhandlers,
                         &state);
}

static void virtio_wifi_device_unrealize(DeviceState* dev, Error** errp) {
    VirtIODevice* vdev = VIRTIO_DEVICE(dev);
    VirtIOWifi* n = VIRTIO_WIFI(dev);
    for (size_t index = 0; index < kQueueSize; index++) {
        virtio_del_queue(vdev, index * 2 + 0);
        virtio_del_queue(vdev, index * 2 + 1);
        qemu_bh_delete(n->vqs[index].tx_bh);
        n->vqs[index].tx_bh = NULL;
    }
    delete[] n->vqs;
    globalState()->wifiService->stop();
    virtio_cleanup(vdev);
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
    // register to the WifiService does not have to pass back
    // an opaque pointer which is essentially VirtIOWifi ptr.
    globalState()->wifi = n;
}

static bool virtio_wifi_started(VirtIOWifi *n, uint8_t status)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(n);
    return (status & VIRTIO_CONFIG_S_DRIVER_OK) &&
        (n->status & VIRTIO_LINK_UP) && vdev->vm_running;
}

static void virtio_wifi_set_status(VirtIODevice* vdev, uint8_t status) {
    VirtIOWifi* n = VIRTIO_WIFI(vdev);
    for (size_t i = 0; i < kQueueSize; i++) {
        VirtIOWifiQueue* q = &n->vqs[i];
        if (virtio_wifi_started(n, status)) {
            qemu_bh_schedule(q->tx_bh);
        }
        else {
            qemu_bh_cancel(q->tx_bh);
            if (status & VIRTIO_CONFIG_S_DRIVER_OK &&
                vdev->vm_running) {
                virtio_queue_set_notification(q->tx, 1);
            }
        }
    }
}

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
    vdc->get_features = virtio_wifi_get_features;
    vdc->set_features = virtio_wifi_set_features;
    vdc->bad_features = virtio_wifi_bad_features;
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
