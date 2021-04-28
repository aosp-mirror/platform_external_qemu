// /*
//  * Licensed to the Apache Software Foundation (ASF) under one
//  * or more contributor license agreements.  See the NOTICE file
//  * distributed with this work for additional information
//  * regarding copyright ownership.  The ASF licenses this file
//  * to you under the Apache License, Version 2.0 (the
//  * "License"); you may not use this file except in compliance
//  * with the License.  You may obtain a copy of the License at
//  *
//  *  http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing,
//  * software distributed under the License is distributed on an
//  * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  * KIND, either express or implied.  See the License for the
//  * specific language governing permissions and limitations
//  * under the License.
//  */

// // TODO(jansene): hook these up to root canal / emu
// #include "hci_bridge.h"

// #include <assert.h>  // for assert
// #include <stdint.h>  // for uint8_t, uint16_t
// #include <stdio.h>   // for NULL
// #include <string.h>  // for memcpy

// #include "android/base/Log.h"
// #include "android/base/sockets/SocketUtils.h"
// #include "nimble/ble.h"            // for BLE_MBUF_MEMBLOCK_OVERHEAD, ble_mb...
// #include "nimble/ble_hci_trans.h"  // for BLE_HCI_TRANS_BUF_EVT_HI, BLE_HCI_...
// #include "nimble/hci_common.h"     // for BLE_HCI_DATA_HDR_SZ, BLE_HCI_EVCOD...
// #include "os/os.h"                 // for os_mempool_init, os_memblock_from
// #include "os/os_mbuf.h"            // for os_mbuf_append, os_mbuf_free_chain
// #include "syscfg/syscfg.h"         // for MYNEWT_VAL, MYNEWT_VAL_BLE_HCI_EVT...
// #include "sysinit/sysinit.h"       // for SYSINIT_PANIC_ASSERT
// #include "model/devices/h4_packetizer.h"

// #include <thread>
// /* HCI packet types */
// #define HCI_PKT_CMD 0x01
// #define HCI_PKT_ACL 0x02
// #define HCI_PKT_EVT 0x04
// #define HCI_PKT_GTL 0x05

// /* Buffers for HCI commands data */
// static uint8_t trans_buf_cmd[BLE_HCI_TRANS_CMD_SZ];
// static uint8_t trans_buf_cmd_allocd;

// #define BTPROTO_HCI 1
// #define HCI_CHANNEL_RAW 0
// #define HCI_CHANNEL_USER 1
// #define HCIDEVUP _IOW('H', 201, int)
// #define HCIDEVDOWN _IOW('H', 202, int)
// #define HCIDEVRESET _IOW('H', 203, int)
// #define HCIGETDEVLIST _IOR('H', 210, int)

// // 'Number of low-priority event buffers.'
// #define BLE_HCI_EVT_LO_BUF_COUNT 8

// /* Buffers for HCI events data */
// static uint8_t trans_buf_evt_hi_pool_buf[OS_MEMPOOL_BYTES(
//         MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
//         MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))];
// static struct os_mempool trans_buf_evt_hi_pool;
// static uint8_t trans_buf_evt_lo_pool_buf[OS_MEMPOOL_BYTES(
//         MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
//         MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))];
// static struct os_mempool trans_buf_evt_lo_pool;

// /* Buffers for HCI ACL data */
// #define ACL_POOL_BLOCK_SIZE                                              \
//     OS_ALIGN(MYNEWT_VAL(BLE_ACL_BUF_SIZE) + BLE_MBUF_MEMBLOCK_OVERHEAD + \
//                      BLE_HCI_DATA_HDR_SZ,                                \
//              OS_ALIGNMENT)
// static uint8_t
//         trans_buf_acl_pool_buf[OS_MEMPOOL_BYTES(MYNEWT_VAL(BLE_ACL_BUF_COUNT),
//                                                 ACL_POOL_BLOCK_SIZE)];
// static struct os_mempool trans_buf_acl_pool;
// static struct os_mbuf_pool trans_buf_acl_mbuf_pool;

// /* Host interface */
// static ble_hci_trans_rx_cmd_fn* trans_rx_cmd_cb;
// static void* trans_rx_cmd_arg;
// static ble_hci_trans_rx_acl_fn* trans_rx_acl_cb;
// static void* trans_rx_acl_arg;

// int g_hciSocket;
// std::unique_ptr<test_vendor_lib::H4Packetizer> g_h4Packetizer;

// int hciSocket() {
//     return g_hciSocket;
// }

// /* Called by NimBLE host to reset HCI transport state (i.e. on host reset) */
// int ble_hci_trans_reset(void) {
//     return 0;
// }

// /* Called by NimBLE host to setup callbacks from HCI transport */
// void ble_hci_trans_cfg_hs(ble_hci_trans_rx_cmd_fn* cmd_cb,
//                           void* cmd_arg,
//                           ble_hci_trans_rx_acl_fn* acl_cb,
//                           void* acl_arg) {
//     trans_rx_cmd_cb = cmd_cb;
//     trans_rx_cmd_arg = cmd_arg;
//     trans_rx_acl_cb = acl_cb;
//     trans_rx_acl_arg = acl_arg;
// }

// /*
//  * Called by NimBLE host to allocate buffer for HCI Command packet.
//  * Called by HCI transport to allocate buffer for HCI Event packet.
//  */
// uint8_t* ble_hci_trans_buf_alloc(int type) {
//     uint8_t* buf;

//     switch (type) {
//         case BLE_HCI_TRANS_BUF_CMD:
//             assert(!trans_buf_cmd_allocd);
//             trans_buf_cmd_allocd = 1;
//             buf = trans_buf_cmd;
//             break;
//         case BLE_HCI_TRANS_BUF_EVT_HI:
//             buf = (uint8_t*)os_memblock_get(&trans_buf_evt_hi_pool);
//             if (buf) {
//                 break;
//             }
//             /* no break */
//         case BLE_HCI_TRANS_BUF_EVT_LO:
//             buf = (uint8_t*)os_memblock_get(&trans_buf_evt_lo_pool);
//             break;
//         default:
//             assert(0);
//             buf = NULL;
//     }

//     return buf;
// }

// /*
//  * Called by NimBLE host to free buffer allocated for HCI Event packet.
//  * Called by HCI transport to free buffer allocated for HCI Command packet.
//  */
// void ble_hci_trans_buf_free(uint8_t* buf) {
//     int rc;

//     if (buf == trans_buf_cmd) {
//         assert(trans_buf_cmd_allocd);
//         trans_buf_cmd_allocd = 0;
//     } else if (os_memblock_from(&trans_buf_evt_hi_pool, buf)) {
//         rc = os_memblock_put(&trans_buf_evt_hi_pool, buf);
//         assert(rc == 0);
//     } else {
//         assert(os_memblock_from(&trans_buf_evt_lo_pool, buf));
//         rc = os_memblock_put(&trans_buf_evt_lo_pool, buf);
//         assert(rc == 0);
//     }
// }

// /* Called by NimBLE host to send HCI Command packet over HCI transport */
// int ble_hci_trans_hs_cmd_tx(uint8_t* cmd) {
//     uint8_t* hci_ev = cmd;
//     uint8_t h4_type = HCI_PKT_CMD;
//     /*
//      * TODO Send HCI Command packet somewhere.
//      * Buffer pointed by 'cmd' contains complete HCI Command packet as defined
//      * by Core spec.
//      */

//     // ble_hci_sock_cmdevt_tx(cmd, BLE_HCI_UART_H4_CMD);

//     // int type = HCI_PKT_CMD;
//     // g_hciSocket->send(&type, sizeof(type));
//     // g_hciSocket->send()
//     // H4 Packetize.
//     // Send to gRPC filters
//     // Send to emulator

//     uint8_t* buf;
//     size_t len;
//     int i;

//     if (h4_type == HCI_PKT_CMD) {
//         len = sizeof(struct ble_hci_cmd) + hci_ev[2];
//         // STATS_INC(hci_sock_stats, ocmd);
//     } else if (h4_type == HCI_PKT_EVT) {
//         len = sizeof(struct ble_hci_ev) + hci_ev[1];
//         // STATS_INC(hci_sock_stats, oevt);
//     } else {
//         assert(0);
//     }

//     // STATS_INC(hci_sock_stats, omsg);
//     // STATS_INCN(hci_sock_stats, obytes, len + 1);

//     buf = (uint8_t*)malloc(len + 1);

//     buf[0] = h4_type;
//     memcpy(&buf[1], hci_ev, len);

//     LOG(INFO) << "Cmd to guest: " << len;

//     i = android::base::socketSend(g_hciSocket, buf, len + 1);

//     free(buf);
//     ble_hci_trans_buf_free(hci_ev);
//     if (i != len + 1) {
//         if (i < 0) {
//             dprintf(1, "sendto() failed : %d\n", errno);
//         } else {
//             dprintf(1, "sendto() partial write: %d != %d\n", i, len + 1);
//         }
//         // STATS_INC(hci_sock_stats, oerr);
//         return BLE_ERR_MEM_CAPACITY;
//     }

//     return 0;
// }

// /* Called by NimBLE host to send HCI ACL Data packet over HCI transport */
// int ble_hci_trans_hs_acl_tx(struct os_mbuf* om) {
//     LOG(INFO) << "Acl to guest: ";
//     // uint8_t *buf = om->om_data;
//     uint8_t h4_type = HCI_PKT_ACL;

//     // H4 Packetize.
//     // Send to gRPC filters
//     // Send to emulator

//     /*
//      * TODO Send HCI ACL Data packet somewhere.
//      * mbuf pointed by 'om' contains complete HCI ACL Data packet as defined
//      * by Core spec.
//      */
//     // (void)buf;

//     int i = android::base::socketSend(g_hciSocket, &h4_type, sizeof(h4_type));
//     int expected = 1;

//     struct os_mbuf* m;
//     uint8_t ch;

//     for (m = om; m; m = SLIST_NEXT(m, om_next)) {
//         i += android::base::socketSend(g_hciSocket, m->om_data, m->om_len);
//         expected += m->om_len;
//     }

//     if (i != expected) {
//         if (i < 0) {
//             dprintf(1, "sendmsg() failed : %d\n", errno);
//         } else {
//             dprintf(1, "sendmsg() partial write: %d\n", i);
//         }
//         return BLE_ERR_MEM_CAPACITY;
//     }
//     return 0;

//     os_mbuf_free_chain(om);

//     return 0;
// }

// /* Called by application to send HCI ACL Data packet to host */
// int hci_transport_send_acl_to_host(const uint8_t* buf, uint16_t size) {
//     LOG(INFO) << "Acl to host: " << size;
//     struct os_mbuf* trans_mbuf;
//     int rc;

//     trans_mbuf = os_mbuf_get_pkthdr(&trans_buf_acl_mbuf_pool,
//                                     sizeof(struct ble_mbuf_hdr));
//     os_mbuf_append(trans_mbuf, buf, size);
//     rc = trans_rx_acl_cb(trans_mbuf, trans_rx_acl_arg);

//     return rc;
// }

// /* Called by application to send HCI Event packet to host */
// int hci_transport_send_evt_to_host(const uint8_t* buf, uint8_t size) {
//     LOG(INFO) << "Packet to host: " << (int) size;
//     uint8_t* trans_buf;
//     int rc;

//     /* Allocate LE Advertising Report Event from lo pool only */
//     if ((buf[0] == BLE_HCI_EVCODE_LE_META) &&
//         (buf[2] == BLE_HCI_LE_SUBEV_ADV_RPT)) {
//         trans_buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_LO);
//         if (!trans_buf) {
//             /* Skip advertising report if we're out of memory */
//             return 0;
//         }
//     } else {
//         trans_buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_HI);
//     }

//     memcpy(trans_buf, buf, size);

//     rc = trans_rx_cmd_cb(trans_buf, trans_rx_cmd_arg);
//     if (rc != 0) {
//         ble_hci_trans_buf_free(trans_buf);
//     }

//     return rc;
// }

// void g4dRreadEvent() {
//     g_h4Packetizer->OnDataReady(g_hciSocket);
// }

// /* Called by application to initialize transport structures */
// int hci_transport_init(void) {
//     int rc;

//     trans_buf_cmd_allocd = 0;

//     rc = os_mempool_init(&trans_buf_acl_pool, MYNEWT_VAL(BLE_ACL_BUF_COUNT),
//                          ACL_POOL_BLOCK_SIZE, trans_buf_acl_pool_buf,
//                          "dummy_hci_acl_pool");
//     SYSINIT_PANIC_ASSERT(rc == 0);

//     rc = os_mbuf_pool_init(&trans_buf_acl_mbuf_pool, &trans_buf_acl_pool,
//                            ACL_POOL_BLOCK_SIZE, MYNEWT_VAL(BLE_ACL_BUF_COUNT));
//     SYSINIT_PANIC_ASSERT(rc == 0);

//     rc = os_mempool_init(
//             &trans_buf_evt_hi_pool, MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
//             MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE), trans_buf_evt_hi_pool_buf,
//             "dummy_hci_hci_evt_hi_pool");
//     SYSINIT_PANIC_ASSERT(rc == 0);

//     rc = os_mempool_init(
//             &trans_buf_evt_lo_pool, MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
//             MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE), trans_buf_evt_lo_pool_buf,
//             "dummy_hci_hci_evt_lo_pool");
//     SYSINIT_PANIC_ASSERT(rc == 0);

//     int port = 6402;
//     g_hciSocket = 0;
//     while (g_hciSocket <= 0) {
//         g_hciSocket = android::base::socketTcp4LoopbackClient(port);
//         if (g_hciSocket < 0) {
//             g_hciSocket = android::base::socketTcp6LoopbackClient(port);
//         }
//         if (g_hciSocket < 0) {
//             LOG(ERROR) << "Failed to connect to: " << port;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(2));
//     }
//     LOG(INFO) << "Connected to " << g_hciSocket << ", port: " << port;
//     android::base::socketSetNonBlocking(g_hciSocket);

//     g_h4Packetizer = std::make_unique<test_vendor_lib::H4Packetizer>(
//             g_hciSocket,
//             [](const std::vector<uint8_t>& /* raw_command */) {
//                 LOG(ERROR) << "Unexpected command: command pkt shouldn't be "
//                             "sent as response.";
//             },
//             [](const std::vector<uint8_t>& raw_event) {
//                 // Unused..
//                 hci_transport_send_evt_to_host(raw_event.data(),
//                                                raw_event.size());
//                 // send(vhci_fd, HCI_PKT_EVT, raw_event.data(),
//                 // raw_event.size());
//             },
//             [](const std::vector<uint8_t>& raw_acl) {
//                 // Unused..
//                 // send(vhci_fd, HCI_ACLDATA_PKT, raw_acl.data(),
//                 // raw_acl.size());
//                 hci_transport_send_acl_to_host(raw_acl.data(), raw_acl.size());
//             },
//             [](const std::vector<uint8_t>& raw_sco) {
//                 // Unused..
//                 // send(vhci_fd, HCI_SCODATA_PKT, raw_sco.data(),
//                 // raw_sco.size());
//             },
//             [](const std::vector<uint8_t>& raw_iso) {
//                 // // Unused..
//                 // send(vhci_fd, HCI_ISODATA_PKT, raw_iso.data(),
//                 // raw_iso.size());
//             },
//             []() { LOG(INFO) << "HCI socket device disconnected"; });
//     return 0;
// }
