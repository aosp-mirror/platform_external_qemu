// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* BLE */
#include "transport/ble_hci_grpc.h"
#include "nimble/ble.h"            // for BLE_...
#include "nimble/ble_hci_trans.h"  // for ble_...
#include "nimble/hci_common.h"     // for BLE_...
#include "nimble/nimble_npl.h"     // for ble_...
#include "nimble/nimble_npl_os.h"  // for BLE_...
#include "nimble/os_types.h"       // for os_s...
#include "os/os.h"                 // for os_m...
#include "os/os_mbuf.h"            // for os_m...
#include "os/queue.h"              // for SLIS...
#include "syscfg/syscfg.h"         // for MYNE...
#include "sysinit/sysinit.h"       // for SYSI...

// nimble brings in some undesirable defines..
#undef min
#undef max
//

#include <assert.h>            // for assert
#include <grpcpp/grpcpp.h>     // for Clie...
#include <stdint.h>            // for uint8_t
#include <stdlib.h>            // for malloc
#include <chrono>              // for oper...
#include <condition_variable>  // for cond...
#include <cstring>             // for NULL
#include <memory>              // for uniq...
#include <mutex>               // for cond...
#include <optional>            // for opti...
#include <string>              // for string
#include <thread>              // for slee...
#include <type_traits>         // for remo...
#include <utility>             // for move
#include <vector>              // for vector

#include "android/emulation/control/EmulatorAdvertisement.h"     // for Emul...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "android/grpc/utils/SimpleAsyncGrpc.h"                  // for Simp...
#include "android/utils/debug.h"                                 // for dinfo
#include "emulated_bluetooth.grpc.pb.h"                          // for Emul...
#include "emulated_bluetooth_packets.pb.h"                       // for HCIP...

using namespace android::emulation::bluetooth;
using namespace android::emulation::control;
using namespace grpc;

#define DEBUG 0
/* set  for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("Channel %s (%p):", __func__, this);     \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#endif

// 128 entries should be enough?
#define BLE_GRPC_STACK_SIZE 128

struct os_task ble_grpc_task;

static struct os_mempool ble_hci_grpc_evt_hi_pool;
static os_membuf_t ble_hci_grpc_evt_hi_buf[OS_MEMPOOL_SIZE(
        MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
        MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))];

static struct os_mempool ble_hci_grpc_evt_lo_pool;
static os_membuf_t ble_hci_grpc_evt_lo_buf[OS_MEMPOOL_SIZE(
        MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
        MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))];

static struct os_mempool ble_hci_grpc_cmd_pool;
static os_membuf_t
        ble_hci_grpc_cmd_buf[OS_MEMPOOL_SIZE(1, BLE_HCI_TRANS_CMD_SZ)];

static struct os_mempool ble_hci_grpc_acl_pool;
static struct os_mbuf_pool ble_hci_grpc_acl_mbuf_pool;

#define ACL_BLOCK_SIZE                                                   \
    OS_ALIGN(MYNEWT_VAL(BLE_ACL_BUF_SIZE) + BLE_MBUF_MEMBLOCK_OVERHEAD + \
                     BLE_HCI_DATA_HDR_SZ,                                \
             OS_ALIGNMENT)
/*
 * The MBUF payload size must accommodate the HCI data header size plus the
 * maximum ACL data packet length. The ACL block size is the size of the
 * mbufs we will allocate.
 */

static os_membuf_t
        ble_hci_grpc_acl_buf[OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                                             ACL_BLOCK_SIZE)];

static ble_hci_trans_rx_cmd_fn* ble_hci_grpc_rx_cmd_cb;
static void* ble_hci_grpc_rx_cmd_arg;
static ble_hci_trans_rx_acl_fn* ble_hci_grpc_rx_acl_cb;
static void* ble_hci_grpc_rx_acl_arg;

// Link layer variants (mainly for testing)
static ble_hci_trans_rx_cmd_fn* ble_hci_grpc_rx_cmd_ll_cb = NULL;
static void* ble_hci_grpc_rx_cmd_ll_arg;
static ble_hci_trans_rx_acl_fn* ble_hci_grpc_rx_acl_ll_cb = NULL;
static void* ble_hci_grpc_rx_acl_ll_arg;

static struct ble_hci_grpc_state {
    int sock;
    struct ble_npl_eventq evq;
    struct ble_npl_event ev;
    struct ble_npl_callout timer;

    uint16_t rx_off;
    uint8_t rx_data[512];
} ble_hci_grpc_state;

static struct os_mbuf* ble_hci_trans_acl_buf_alloc(void);

class HciTransport : public SimpleClientBidiStream<HCIPacket, HCIPacket> {
public:
    HciTransport(std::unique_ptr<ClientContext>&& ctx)
        : mContext(std::move(ctx)) {}
    virtual ~HciTransport() {}

    void Read(const HCIPacket* received) override {
        DD("Forwarding %s -> %d", mContext->peer().c_str(),
           received->packet().size());

        switch (received->type()) {
            case HCIPacket::PACKET_TYPE_EVENT: {
                int sr;
                uint8_t* data =
                        ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_HI);
                auto len = 1 + sizeof(struct ble_hci_ev) +
                           received->packet().data()[1];
                DD("%d = %d", len - 1, received->packet().size());
                memcpy(data, received->packet().data(),
                       received->packet().size());
                OS_ENTER_CRITICAL(sr);
                int rc = ble_hci_grpc_rx_cmd_cb(data, ble_hci_grpc_rx_cmd_arg);
                OS_EXIT_CRITICAL(sr);
                if (rc) {
                    ble_hci_trans_buf_free(data);
                }

                break;
            }
            case HCIPacket::PACKET_TYPE_ACL: {
                int sr;
                struct os_mbuf* m = ble_hci_trans_acl_buf_alloc();
                if (!m) {
                    break;
                }
                if (os_mbuf_append(m, received->packet().data(),
                                   received->packet().size())) {
                    os_mbuf_free_chain(m);
                    break;
                }
                OS_ENTER_CRITICAL(sr);
                ble_hci_grpc_rx_acl_cb(m, ble_hci_grpc_rx_acl_arg);
                OS_EXIT_CRITICAL(sr);
                break;
            }
            default:
                dwarning("Unexpected packet: %s",
                         received->ShortDebugString().c_str());
        }
    }

    void OnDone(const grpc::Status& s) override {
        // TODO(jansene): well, hci is dead.. so... what now?
        dfatal("Emulator %s is gone due to: %s", mContext->peer().c_str(),
               s.error_message().c_str());
    }

    // Blocks and waits until this connection has completed.
    grpc::Status await() {
        std::unique_lock<std::mutex> l(mAwaitMutex);
        mAwaitCondition.wait(l, [this] { return mDone; });
        return std::move(mStatus);
    }

    void cancel() { mContext->TryCancel(); }

    void startCall(EmulatedBluetoothService::Stub* stub) {
        stub->async()->registerHCIDevice(mContext.get(), this);
        StartCall();
        StartRead();
    };

protected:
    std::unique_ptr<grpc::ClientContext> mContext;

private:
    std::mutex mAwaitMutex;
    std::condition_variable mAwaitCondition;
    grpc::Status mStatus;
    bool mDone = false;
};

std::unique_ptr<EmulatorGrpcClient> firstEmulatorOrNone() {
    auto emulators = EmulatorAdvertisement({}).discoverRunningEmulators();
    for (const auto& discovered : emulators) {
        auto client = std::make_unique<EmulatorGrpcClient>(discovered);
        if (client->hasOpenChannel()) {
            return client;
        }
    }
    return nullptr;
}

std::unique_ptr<HciTransport> sTransport;
std::unique_ptr<EmulatedBluetoothService::Stub> sTransportStub;
std::unique_ptr<EmulatorGrpcClient> sClient;

void injectGrpcClient(std::unique_ptr<EmulatorGrpcClient> client) {
    sClient = std::move(client);
}

int ble_hci_grpc_open_connection() {
    using namespace std::chrono_literals;

    // This only happens if we didn't inject a client.
    while (!sClient) {
        dinfo("Waiting for first emulator..");
        std::this_thread::sleep_for(1s);
        sClient = firstEmulatorOrNone();
    }

    sTransportStub = sClient->stub<EmulatedBluetoothService>();
    sTransport = std::make_unique<HciTransport>(sClient->newContext());
    sTransport->startCall(sTransportStub.get());
    return 0;
}

void* ble_hci_grpc_ack_handler(void* arg) {
    struct ble_npl_event* ev;

    while (1) {
        ev = ble_npl_eventq_get(&ble_hci_grpc_state.evq, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
    }
    return NULL;
}

// We don't need an event pump..
static void ble_hci_grpc_init_task(void) {
    ble_npl_eventq_init(&ble_hci_grpc_state.evq);

#if MYNEWT
    {
        os_stack_t* pstack;

        pstack = (os_stack_t*)malloc(sizeof(os_stack_t) * BLE_GRPC_STACK_SIZE);
        assert(pstack);
        os_task_init(&ble_grpc_task, "hci_grpc", ble_hci_grpc_ack_handler, NULL,
                     MYNEWT_VAL(BLE_SOCK_TASK_PRIO), BLE_NPL_TIME_FOREVER,
                     pstack, BLE_GRPC_STACK_SIZE);
    }
#else
    /*
     * For non-Mynewt OS it is required that OS creates task for HCI SOCKET
     * to run ble_hci_grpc_ack_handler.
     */

#endif
}

/**
 * Initializes the UART HCI transport module.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
extern "C" void ble_hci_transport_init(void) {
    // First we need to allocate all thge memory and resources..
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    memset(&ble_hci_grpc_state, 0, sizeof(ble_hci_grpc_state));
    ble_hci_grpc_state.sock = -1;

    // Note: we are not using an event loop, as gRPC will generate events
    // upon receiving messages.

    rc = os_mempool_init(&ble_hci_grpc_acl_pool, MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                         ACL_BLOCK_SIZE, ble_hci_grpc_acl_buf,
                         (char*)"ble_hci_grpc_acl_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mbuf_pool_init(&ble_hci_grpc_acl_mbuf_pool, &ble_hci_grpc_acl_pool,
                           ACL_BLOCK_SIZE, MYNEWT_VAL(BLE_ACL_BUF_COUNT));
    SYSINIT_PANIC_ASSERT(rc == 0);

    /*
     * Create memory pool of HCI command buffers. NOTE: we currently dont
     * allow this to be configured. The controller will only allow one
     * outstanding command. We decided to keep this a pool in case we allow
     * allow the controller to handle more than one outstanding command.
     */
    rc = os_mempool_init(&ble_hci_grpc_cmd_pool, 1, BLE_HCI_TRANS_CMD_SZ,
                         &ble_hci_grpc_cmd_buf, (char*)"ble_hci_grpc_cmd_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(
            &ble_hci_grpc_evt_hi_pool, MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
            MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE), &ble_hci_grpc_evt_hi_buf,
            (char*)"ble_hci_grpc_evt_hi_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(
            &ble_hci_grpc_evt_lo_pool, MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
            MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE), ble_hci_grpc_evt_lo_buf,
            (char*)"ble_hci_grpc_evt_lo_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    // Next we initialize our gRPC connection to the first available emulator.
    ble_hci_grpc_open_connection();
}

/**
 * Sends an HCI event from the controller to the host.
 *
 * @param cmd                   The HCI event to send.  This buffer must be
 *                                  allocated via ble_hci_trans_buf_alloc().
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_ll_evt_tx(uint8_t* cmd) {
    HCIPacket toSend;
    toSend.set_type(HCIPacket::PACKET_TYPE_EVENT);
    auto len = sizeof(struct ble_hci_ev) + cmd[1];
    toSend.set_packet(cmd, len);
    ble_hci_trans_buf_free(cmd);
    if (!sTransport) {
        return BLE_ERR_HW_FAIL;
    }
    sTransport->Write(toSend);
    return 0;
}

/**
 * Allocates a buffer (mbuf) for ACL operation.
 *
 * @return                      The allocated buffer on success;
 *                              NULL on buffer exhaustion.
 */
static struct os_mbuf* ble_hci_trans_acl_buf_alloc(void) {
    struct os_mbuf* m;

    /*
     * XXX: note that for host only there would be no need to allocate
     * a user header. Address this later.
     */
    m = os_mbuf_get_pkthdr(&ble_hci_grpc_acl_mbuf_pool,
                           sizeof(struct ble_mbuf_hdr));
    return m;
}

/**
 * Sends ACL data from controller to host.
 *
 * @param om                    The ACL data packet to send.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_ll_acl_tx(struct os_mbuf* om) {
    return ble_hci_trans_hs_acl_tx(om);
}

/**
 * Sends an HCI command from the host to the controller.
 *
 * @param cmd                   The HCI command to send.  This buffer must be
 *                                  allocated via ble_hci_trans_buf_alloc().
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_hs_cmd_tx(uint8_t* cmd) {
    HCIPacket toSend;
    toSend.set_type(HCIPacket::PACKET_TYPE_HCI_COMMAND);
    auto len = sizeof(struct ble_hci_cmd) + cmd[2];
    toSend.set_packet(cmd, len);
    ble_hci_trans_buf_free(cmd);
    if (!sTransport) {
        return BLE_ERR_HW_FAIL;
    }
    sTransport->Write(toSend);
    return 0;
}

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_hs_acl_tx(struct os_mbuf* om) {
    int i;
    struct os_mbuf* m;
    uint8_t ch;
    std::string packet;

    HCIPacket toSend;
    toSend.set_type(HCIPacket::PACKET_TYPE_ACL);

    for (m = om; m; m = SLIST_NEXT(m, om_next)) {
        packet.append((char*)m->om_data, m->om_len);
    }

    toSend.set_packet(std::move(packet));
    if (!sTransport) {
        return BLE_ERR_HW_FAIL;
    }
    sTransport->Write(toSend);

    if (ble_hci_grpc_rx_acl_ll_cb) {
        ble_hci_grpc_rx_acl_ll_cb(om, ble_hci_grpc_rx_acl_ll_arg);
    }
    os_mbuf_free_chain(om);
    return 0;
}

/**
 * Configures the HCI transport to call the specified callback upon receiving
 * HCI packets from the controller.  This function should only be called by by
 * host.
 *
 * @param cmd_cb                The callback to execute upon receiving an HCI
 *                                  event.
 * @param cmd_arg               Optional argument to pass to the command
 *                                  callback.
 * @param acl_cb                The callback to execute upon receiving ACL
 *                                  data.
 * @param acl_arg               Optional argument to pass to the ACL
 *                                  callback.
 */
void ble_hci_trans_cfg_hs(ble_hci_trans_rx_cmd_fn* cmd_cb,
                          void* cmd_arg,
                          ble_hci_trans_rx_acl_fn* acl_cb,
                          void* acl_arg) {
    ble_hci_grpc_rx_cmd_cb = cmd_cb;
    ble_hci_grpc_rx_cmd_arg = cmd_arg;
    ble_hci_grpc_rx_acl_cb = acl_cb;
    ble_hci_grpc_rx_acl_arg = acl_arg;
}

/**
 * Configures the HCI transport to operate with a host.  The transport will
 * execute specified callbacks upon receiving HCI packets from the controller.
 *
 * @param cmd_cb                The callback to execute upon receiving an HCI
 *                                  event.
 * @param cmd_arg               Optional argument to pass to the command
 *                                  callback.
 * @param acl_cb                The callback to execute upon receiving ACL
 *                                  data.
 * @param acl_arg               Optional argument to pass to the ACL
 *                                  callback.
 */
void ble_hci_trans_cfg_ll(ble_hci_trans_rx_cmd_fn* cmd_cb,
                          void* cmd_arg,
                          ble_hci_trans_rx_acl_fn* acl_cb,
                          void* acl_arg) {
    ble_hci_grpc_rx_cmd_ll_cb = cmd_cb;
    ble_hci_grpc_rx_cmd_ll_arg = cmd_arg;
    ble_hci_grpc_rx_acl_ll_cb = acl_cb;
    ble_hci_grpc_rx_acl_ll_arg = acl_arg;
}

/**
 * Allocates a flat buffer of the specified type.
 *
 * @param type                  The type of buffer to allocate; one of the
 *                                  BLE_HCI_TRANS_BUF_[...] constants.
 *
 * @return                      The allocated buffer on success;
 *                              NULL on buffer exhaustion.
 */
uint8_t* ble_hci_trans_buf_alloc(int type) {
    void* buf;

    switch (type) {
        case BLE_HCI_TRANS_BUF_CMD:
            buf = os_memblock_get(&ble_hci_grpc_cmd_pool);
            break;
        case BLE_HCI_TRANS_BUF_EVT_HI:
            buf = os_memblock_get(&ble_hci_grpc_evt_hi_pool);
            if (buf == NULL) {
                /* If no high-priority event buffers remain, try to grab a
                 * low-priority one.
                 */
                buf = os_memblock_get(&ble_hci_grpc_evt_lo_pool);
            }
            break;

        case BLE_HCI_TRANS_BUF_EVT_LO:
            buf = os_memblock_get(&ble_hci_grpc_evt_lo_pool);
            break;

        default:
            assert(0);
            buf = NULL;
    }

    return reinterpret_cast<uint8_t*>(buf);
}

/**
 * Frees the specified flat buffer.  The buffer must have been allocated via
 * ble_hci_trans_buf_alloc().
 *
 * @param buf                   The buffer to free.
 */
void ble_hci_trans_buf_free(uint8_t* buf) {
    int rc;

    /*
     * XXX: this may look a bit odd, but the controller uses the command
     * buffer to send back the command complete/status as an immediate
     * response to the command. This was done to insure that the controller
     * could always send back one of these events when a command was received.
     * Thus, we check to see which pool the buffer came from so we can free
     * it to the appropriate pool
     */
    if (os_memblock_from(&ble_hci_grpc_evt_hi_pool, buf)) {
        rc = os_memblock_put(&ble_hci_grpc_evt_hi_pool, buf);
        assert(rc == 0);
    } else if (os_memblock_from(&ble_hci_grpc_evt_lo_pool, buf)) {
        rc = os_memblock_put(&ble_hci_grpc_evt_lo_pool, buf);
        assert(rc == 0);
    } else {
        assert(os_memblock_from(&ble_hci_grpc_cmd_pool, buf));
        rc = os_memblock_put(&ble_hci_grpc_cmd_pool, buf);
        assert(rc == 0);
    }
}

/**
 * Resets the HCI UART transport to a clean state.  Frees all buffers and
 * reconfigures the UART.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_reset(void) {
    /* Close the connection. */
    if (sTransport) {
        dinfo("Cancelling the transport.");
        sTransport->cancel();
    }

    dfatal("The stack is performing a reset.. this is not (yet?) "
           "supported");
    return 0;
}
