// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/bluetooth/nimble/TypeTranslator.h"

#include <stdio.h>  // for sprintf, NULL

#include "host/ble_gap.h"   // for ble_gap_conn_find, BLE_GAP_DISC_MODE_GEN
#include "host/ble_gatt.h"  // for BLE_GATT_SVC_TYPE_PRIMARY, BLE_GATT_SVC_T...
#include "nimble/ble.h"     // for ble_addr_t
#include "os/os_mbuf.h"     // for os_mbuf, os_mbuf::(anonymous)
#include "os/queue.h"       // for SLIST_NEXT

namespace android {
namespace emulation {
namespace bluetooth {

ble_uuid_any_t toNimbleUuid(Uuid uuid) {
    ble_uuid_any_t val = {0};
    if (uuid.has_lsb()) {
        // Most definitely a long uuid.
        uint64_t* uuidPtr = (uint64_t*)val.u128.value;
        uuidPtr[0] = uuid.lsb();
        uuidPtr[1] = uuid.msb();
        val.u128.u.type = BLE_UUID_TYPE_128;
    } else if (uuid.id() < (0xFFFF + 1)) {
        // 16 bit
         val.u16.u.type = BLE_UUID_TYPE_16;
         val.u16.value = (uint16_t)uuid.id();
    } else {
         val.u32.u.type = BLE_UUID_TYPE_32;
         val.u32.value = (uint32_t)uuid.id();
    }
    return val;
}

uint8_t toNimbleServiceType(GattService::ServiceType type) {
    switch (type) {
        case GattService::SERVICE_TYPE_UNSPECIFIED:
            return BLE_GATT_SVC_TYPE_PRIMARY;
        case GattService::SERVICE_TYPE_PRIMARY:
            return BLE_GATT_SVC_TYPE_PRIMARY;
        case GattService::SERVICE_TYPE_SECONDARY:
            return BLE_GATT_SVC_TYPE_SECONDARY;
        default:
            return BLE_GATT_SVC_TYPE_PRIMARY;
    }
}

uint8_t toNimbleDiscoveryMode(const Advertisement::DiscoveryMode mode) {
    switch (mode) {
        // Non-discoverable, as per section 3.C.9.2.2).
        case Advertisement::DISCOVERY_MODE_NON_DISCOVERABLE:
            // Limited-discoverable, as per section 3.C.9.2.3).
            return BLE_GAP_DISC_MODE_NON;
        case Advertisement::DISCOVERY_MODE_LIMITED:
            return BLE_GAP_DISC_MODE_LTD;
            // General-discoverable, as per section 3.C.9.2.4).
        case Advertisement::DISCOVERY_MODE_GENERAL:
            return BLE_GAP_DISC_MODE_GEN;
        default:
            return BLE_GAP_DISC_MODE_GEN;
    }
}

DeviceIdentifier lookupDeviceIdentifier(uint16_t conn_handle) {
    struct ble_gap_conn_desc desc = {0};
    ble_gap_conn_find(conn_handle, &desc);

    /* 6 bytes of MAC address * 2 signs for each byte in string + 5 colons to
     * separate bytes + 1 byte for null-character appended by sprintf
     */
    char buf[6 * 2 + 5 + 1];
    const uint8_t* u8p = desc.peer_id_addr.val;
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3],
            u8p[2], u8p[1], u8p[0]);

    DeviceIdentifier id;
    id.set_address(buf);
    return id;
}

std::string collectOmChain(struct os_mbuf* om) {
    std::string data;
    while (om != NULL) {
        data.append(std::string((char*)om->om_data, om->om_len));
        om = SLIST_NEXT(om, om_next);
    }
    return data;
}

uint8_t toNimbleConnectionMode(const Advertisement::ConnectionMode mode) {
    switch (mode) {
            // Non-connectable, as per section 3.C.9.3.2
        case Advertisement::CONNECTION_MODE_NON_CONNECTABLE:
            return BLE_GAP_CONN_MODE_NON;
        // Directed-connectable, as per section 3.C.9.3.3
        case Advertisement::CONNECTION_MODE_DIRECTED:
            return BLE_GAP_CONN_MODE_DIR;
        // Undirected-connectable, as per section 3.C.9.3.4
        case Advertisement::CONNECTION_MODE_UNDIRECTED:
            return BLE_GAP_CONN_MODE_UND;
        default:
            return BLE_GAP_CONN_MODE_UND;
    }
}

static std::string bleErrcodeToString(ble_error_codes rc) {
    switch (rc) {
        case BLE_ERR_SUCCESS:
            return "BLE_ERR_SUCCESS";
        case BLE_ERR_UNKNOWN_HCI_CMD:
            return "BLE_ERR_UNKNOWN_HCI_CMD";
        case BLE_ERR_UNK_CONN_ID:
            return "BLE_ERR_UNK_CONN_ID";
        case BLE_ERR_HW_FAIL:
            return "BLE_ERR_HW_FAIL";
        case BLE_ERR_PAGE_TMO:
            return "BLE_ERR_PAGE_TMO";
        case BLE_ERR_AUTH_FAIL:
            return "BLE_ERR_AUTH_FAIL";
        case BLE_ERR_PINKEY_MISSING:
            return "BLE_ERR_PINKEY_MISSING";
        case BLE_ERR_MEM_CAPACITY:
            return "BLE_ERR_MEM_CAPACITY";
        case BLE_ERR_CONN_SPVN_TMO:
            return "BLE_ERR_CONN_SPVN_TMO";
        case BLE_ERR_CONN_LIMIT:
            return "BLE_ERR_CONN_LIMIT";
        case BLE_ERR_SYNCH_CONN_LIMIT:
            return "BLE_ERR_SYNCH_CONN_LIMIT";
        case BLE_ERR_ACL_CONN_EXISTS:
            return "BLE_ERR_ACL_CONN_EXISTS";
        case BLE_ERR_CMD_DISALLOWED:
            return "BLE_ERR_CMD_DISALLOWED";
        case BLE_ERR_CONN_REJ_RESOURCES:
            return "BLE_ERR_CONN_REJ_RESOURCES";
        case BLE_ERR_CONN_REJ_SECURITY:
            return "BLE_ERR_CONN_REJ_SECURITY";
        case BLE_ERR_CONN_REJ_BD_ADDR:
            return "BLE_ERR_CONN_REJ_BD_ADDR";
        case BLE_ERR_CONN_ACCEPT_TMO:
            return "BLE_ERR_CONN_ACCEPT_TMO";
        case BLE_ERR_UNSUPPORTED:
            return "BLE_ERR_UNSUPPORTED";
        case BLE_ERR_INV_HCI_CMD_PARMS:
            return "BLE_ERR_INV_HCI_CMD_PARMS";
        case BLE_ERR_REM_USER_CONN_TERM:
            return "BLE_ERR_REM_USER_CONN_TERM";
        case BLE_ERR_RD_CONN_TERM_RESRCS:
            return "BLE_ERR_RD_CONN_TERM_RESRCS";
        case BLE_ERR_RD_CONN_TERM_PWROFF:
            return "BLE_ERR_RD_CONN_TERM_PWROFF";
        case BLE_ERR_CONN_TERM_LOCAL:
            return "BLE_ERR_CONN_TERM_LOCAL";
        case BLE_ERR_REPEATED_ATTEMPTS:
            return "BLE_ERR_REPEATED_ATTEMPTS";
        case BLE_ERR_NO_PAIRING:
            return "BLE_ERR_NO_PAIRING";
        case BLE_ERR_UNK_LMP:
            return "BLE_ERR_UNK_LMP";
        case BLE_ERR_UNSUPP_REM_FEATURE:
            return "BLE_ERR_UNSUPP_REM_FEATURE";
        case BLE_ERR_SCO_OFFSET:
            return "BLE_ERR_SCO_OFFSET";
        case BLE_ERR_SCO_ITVL:
            return "BLE_ERR_SCO_ITVL";
        case BLE_ERR_SCO_AIR_MODE:
            return "BLE_ERR_SCO_AIR_MODE";
        case BLE_ERR_INV_LMP_LL_PARM:
            return "BLE_ERR_INV_LMP_LL_PARM";
        case BLE_ERR_UNSPECIFIED:
            return "BLE_ERR_UNSPECIFIED";
        case BLE_ERR_UNSUPP_LMP_LL_PARM:
            return "BLE_ERR_UNSUPP_LMP_LL_PARM";
        case BLE_ERR_NO_ROLE_CHANGE:
            return "BLE_ERR_NO_ROLE_CHANGE";
        case BLE_ERR_LMP_LL_RSP_TMO:
            return "BLE_ERR_LMP_LL_RSP_TMO";
        case BLE_ERR_LMP_COLLISION:
            return "BLE_ERR_LMP_COLLISION";
        case BLE_ERR_LMP_PDU:
            return "BLE_ERR_LMP_PDU";
        case BLE_ERR_ENCRYPTION_MODE:
            return "BLE_ERR_ENCRYPTION_MODE";
        case BLE_ERR_LINK_KEY_CHANGE:
            return "BLE_ERR_LINK_KEY_CHANGE";
        case BLE_ERR_UNSUPP_QOS:
            return "BLE_ERR_UNSUPP_QOS";
        case BLE_ERR_INSTANT_PASSED:
            return "BLE_ERR_INSTANT_PASSED";
        case BLE_ERR_UNIT_KEY_PAIRING:
            return "BLE_ERR_UNIT_KEY_PAIRING";
        case BLE_ERR_DIFF_TRANS_COLL:
            return "BLE_ERR_DIFF_TRANS_COLL";
        case BLE_ERR_QOS_PARM:
            return "BLE_ERR_QOS_PARM";
        case BLE_ERR_QOS_REJECTED:
            return "BLE_ERR_QOS_REJECTED";
        case BLE_ERR_CHAN_CLASS:
            return "BLE_ERR_CHAN_CLASS";
        case BLE_ERR_INSUFFICIENT_SEC:
            return "BLE_ERR_INSUFFICIENT_SEC";
        case BLE_ERR_PARM_OUT_OF_RANGE:
            return "BLE_ERR_PARM_OUT_OF_RANGE";
        case BLE_ERR_PENDING_ROLE_SW:
            return "BLE_ERR_PENDING_ROLE_SW";
        case BLE_ERR_RESERVED_SLOT:
            return "BLE_ERR_RESERVED_SLOT";
        case BLE_ERR_ROLE_SW_FAIL:
            return "BLE_ERR_ROLE_SW_FAIL";
        case BLE_ERR_INQ_RSP_TOO_BIG:
            return "BLE_ERR_INQ_RSP_TOO_BIG";
        case BLE_ERR_SEC_SIMPLE_PAIR:
            return "BLE_ERR_SEC_SIMPLE_PAIR";
        case BLE_ERR_HOST_BUSY_PAIR:
            return "BLE_ERR_HOST_BUSY_PAIR";
        case BLE_ERR_CONN_REJ_CHANNEL:
            return "BLE_ERR_CONN_REJ_CHANNEL";
        case BLE_ERR_CTLR_BUSY:
            return "BLE_ERR_CTLR_BUSY";
        case BLE_ERR_CONN_PARMS:
            return "BLE_ERR_CONN_PARMS";
        case BLE_ERR_DIR_ADV_TMO:
            return "BLE_ERR_DIR_ADV_TMO";
        case BLE_ERR_CONN_TERM_MIC:
            return "BLE_ERR_CONN_TERM_MIC";
        case BLE_ERR_CONN_ESTABLISHMENT:
            return "BLE_ERR_CONN_ESTABLISHMENT";
        case BLE_ERR_MAC_CONN_FAIL:
            return "BLE_ERR_MAC_CONN_FAIL";
        case BLE_ERR_COARSE_CLK_ADJ:
            return "BLE_ERR_COARSE_CLK_ADJ";
        case BLE_ERR_TYPE0_SUBMAP_NDEF:
            return "BLE_ERR_TYPE0_SUBMAP_NDEF";
        case BLE_ERR_UNK_ADV_INDENT:
            return "BLE_ERR_UNK_ADV_INDENT";
        case BLE_ERR_LIMIT_REACHED:
            return "BLE_ERR_LIMIT_REACHED";
        case BLE_ERR_OPERATION_CANCELLED:
            return "BLE_ERR_OPERATION_CANCELLED";
        case BLE_ERR_PACKET_TOO_LONG:
            return "BLE_ERR_PACKET_TOO_LONG";
        default:
            return "Error Unknown: " + std::to_string(rc);
    }
}

static std::string bleAttErrorToString(int rc) {
    switch (rc) {
        case BLE_ATT_OP_ERROR_RSP:
            return "BLE_ATT_OP_ERROR_RSP";
        case BLE_ATT_OP_MTU_REQ:
            return "BLE_ATT_OP_MTU_REQ";
        case BLE_ATT_OP_MTU_RSP:
            return "BLE_ATT_OP_MTU_RSP";
        case BLE_ATT_OP_FIND_INFO_REQ:
            return "BLE_ATT_OP_FIND_INFO_REQ";
        case BLE_ATT_OP_FIND_INFO_RSP:
            return "BLE_ATT_OP_FIND_INFO_RSP";
        case BLE_ATT_OP_FIND_TYPE_VALUE_REQ:
            return "BLE_ATT_OP_FIND_TYPE_VALUE_REQ";
        case BLE_ATT_OP_FIND_TYPE_VALUE_RSP:
            return "BLE_ATT_OP_FIND_TYPE_VALUE_RSP";
        case BLE_ATT_OP_READ_TYPE_REQ:
            return "BLE_ATT_OP_READ_TYPE_REQ";
        case BLE_ATT_OP_READ_TYPE_RSP:
            return "BLE_ATT_OP_READ_TYPE_RSP";
        case BLE_ATT_OP_READ_REQ:
            return "BLE_ATT_OP_READ_REQ";
        case BLE_ATT_OP_READ_RSP:
            return "BLE_ATT_OP_READ_RSP";
        case BLE_ATT_OP_READ_BLOB_REQ:
            return "BLE_ATT_OP_READ_BLOB_REQ";
        case BLE_ATT_OP_READ_BLOB_RSP:
            return "BLE_ATT_OP_READ_BLOB_RSP";
        case BLE_ATT_OP_READ_MULT_REQ:
            return "BLE_ATT_OP_READ_MULT_REQ";
        case BLE_ATT_OP_READ_MULT_RSP:
            return "BLE_ATT_OP_READ_MULT_RSP";
        case BLE_ATT_OP_READ_GROUP_TYPE_REQ:
            return "BLE_ATT_OP_READ_GROUP_TYPE_REQ";
        case BLE_ATT_OP_READ_GROUP_TYPE_RSP:
            return "BLE_ATT_OP_READ_GROUP_TYPE_RSP";
        case BLE_ATT_OP_WRITE_REQ:
            return "BLE_ATT_OP_WRITE_REQ";
        case BLE_ATT_OP_WRITE_RSP:
            return "BLE_ATT_OP_WRITE_RSP";
        case BLE_ATT_OP_PREP_WRITE_REQ:
            return "BLE_ATT_OP_PREP_WRITE_REQ";
        case BLE_ATT_OP_PREP_WRITE_RSP:
            return "BLE_ATT_OP_PREP_WRITE_RSP";
        case BLE_ATT_OP_EXEC_WRITE_REQ:
            return "BLE_ATT_OP_EXEC_WRITE_REQ";
        case BLE_ATT_OP_EXEC_WRITE_RSP:
            return "BLE_ATT_OP_EXEC_WRITE_RSP";
        case BLE_ATT_OP_NOTIFY_REQ:
            return "BLE_ATT_OP_NOTIFY_REQ";
        case BLE_ATT_OP_INDICATE_REQ:
            return "BLE_ATT_OP_INDICATE_REQ";
        case BLE_ATT_OP_INDICATE_RSP:
            return "BLE_ATT_OP_INDICATE_RSP";
        case BLE_ATT_OP_WRITE_CMD:
            return "BLE_ATT_OP_WRITE_CMD";
        default:
            return "Unknown ble_att error: " + std::to_string(rc);
    }
}

static std::string bleHostErrorToString(int rc) {
    switch (rc) {
        case BLE_HS_EAGAIN:
            return "BLE_HS_EAGAIN";
        case BLE_HS_EALREADY:
            return "BLE_HS_EALREADY";
        case BLE_HS_EINVAL:
            return "BLE_HS_EINVAL";
        case BLE_HS_EMSGSIZE:
            return "BLE_HS_EMSGSIZE";
        case BLE_HS_ENOENT:
            return "BLE_HS_ENOENT";
        case BLE_HS_ENOMEM:
            return "BLE_HS_ENOMEM";
        case BLE_HS_ENOTCONN:
            return "BLE_HS_ENOTCONN";
        case BLE_HS_ENOTSUP:
            return "BLE_HS_ENOTSUP";
        case BLE_HS_EAPP:
            return "BLE_HS_EAPP";
        case BLE_HS_EBADDATA:
            return "BLE_HS_EBADDATA";
        case BLE_HS_EOS:
            return "BLE_HS_EOS";
        case BLE_HS_ECONTROLLER:
            return "BLE_HS_ECONTROLLER";
        case BLE_HS_ETIMEOUT:
            return "BLE_HS_ETIMEOUT";
        case BLE_HS_EDONE:
            return "BLE_HS_EDONE";
        case BLE_HS_EBUSY:
            return "BLE_HS_EBUSY";
        case BLE_HS_EREJECT:
            return "BLE_HS_EREJECT";
        case BLE_HS_EUNKNOWN:
            return "BLE_HS_EUNKNOWN";
        case BLE_HS_EROLE:
            return "BLE_HS_EROLE";
        case BLE_HS_ETIMEOUT_HCI:
            return "BLE_HS_ETIMEOUT_HCI";
        case BLE_HS_ENOMEM_EVT:
            return "BLE_HS_ENOMEM_EVT";
        case BLE_HS_ENOADDR:
            return "BLE_HS_ENOADDR";
        case BLE_HS_ENOTSYNCED:
            return "BLE_HS_ENOTSYNCED";
        case BLE_HS_EAUTHEN:
            return "BLE_HS_EAUTHEN";
        case BLE_HS_EAUTHOR:
            return "BLE_HS_EAUTHOR";
        case BLE_HS_EENCRYPT:
            return "BLE_HS_EENCRYPT";
        case BLE_HS_EENCRYPT_KEY_SZ:
            return "BLE_HS_EENCRYPT_KEY_SZ";
        case BLE_HS_ESTORE_CAP:
            return "BLE_HS_ESTORE_CAP";
        case BLE_HS_ESTORE_FAIL:
            return "BLE_HS_ESTORE_FAIL";
        case BLE_HS_EPREEMPTED:
            return "BLE_HS_EPREEMPTED";
        case BLE_HS_EDISABLED:
            return "BLE_HS_EDISABLED";
        case BLE_HS_ESTALLED:
            return "BLE_HS_ESTALLED";
        default:
            return "Unknown ble_hs error: " + std::to_string(rc);
    }
}
std::string bleErrToString(int rc) {
    if (rc < BLE_HS_ERR_ATT_BASE) {
        return bleHostErrorToString(rc);
    }

    if (rc < BLE_HS_ERR_HCI_BASE) {
        return bleAttErrorToString(rc - BLE_HS_ERR_ATT_BASE);
    }

    return "Unknown error code: " + std::to_string(rc);
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
