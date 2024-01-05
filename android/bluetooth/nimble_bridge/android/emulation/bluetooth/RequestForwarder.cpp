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
#include "android/emulation/bluetooth/RequestForwarder.h"

#include "aemu/base/Log.h"
#include "android/base/system/System.h"
#include "android/emulation/bluetooth/RemoteConnection.h"
#include "android/emulation/bluetooth/nimble/TypeTranslator.h"
#include "android/utils/debug.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#undef min
#undef max

extern "C" void sysinit();

namespace android {
namespace emulation {
namespace bluetooth {

std::unique_ptr<RequestForwarder> sRequestForwarder;

static void nimble_sync_cb() {
    if (!sRequestForwarder) {
        dfatal("Unable to start sync.. Device does not exist. (this shouldn't "
               "happen)");
    }

    sRequestForwarder->onSync();
}

RequestForwarder::RequestForwarder(const GattDevice& device)
    : mProfile(device.profile(), this) {
    mDevice.CopyFrom(device);
    mClient = EmulatorGrpcClient::Builder()
                      .withEndpoint(device.endpoint())
                      .build()
                      .value();

    std::string pid =
            std::to_string(android::base::System::get()->getCurrentProcessId());
    mMyId.set_identity(pid);
}

RequestForwarder* RequestForwarder::registerDevice(
        GattDevice device,
        std::unique_ptr<EmulatorGrpcClient> hci_transport) {
    // Initialize OS
    sysinit();

    // Initialize the NimBLE host configuration
    ble_hs_cfg.sync_cb = nimble_sync_cb;

    dinfo("Registering %s", device.ShortDebugString());
    sRequestForwarder = std::make_unique<RequestForwarder>(device);
    return sRequestForwarder.get();
}

void RequestForwarder::start() {
    while (mRunning && mClient->hasOpenChannel()) {
        os_eventq_run(os_eventq_dflt_get());
    }
}

void RequestForwarder::stop() {
    mRunning = false;
}

void RequestForwarder::onSync() {
    // Use privacy
    ble_hs_id_infer_auto(0, &mAddrType);
    advertise();
}

absl::Status RequestForwarder::initialize() {
    if (!mClient->hasOpenChannel()) {
        return absl::Status(absl::StatusCode::kInternal,
                            "Unable to connect to endpoint.");
    }

    auto serviceDef = mProfile.getNimbleServiceDef();
    if (ble_gatts_count_cfg(serviceDef) != 0) {
        return absl::Status(absl::StatusCode::kInternal,
                            "Unable to count service definitions.");
    }
    if (ble_gatts_add_svcs(serviceDef) != 0) {
        return absl::Status(absl::StatusCode::kInternal,
                            "Unable to register service definition.");
    };

    // Note, a bluetooth name can never be > 31 chars.
    auto device_name = mDevice.advertisement().device_name().substr(0, 31);
    if (ble_svc_gap_device_name_set(device_name.c_str()) != 0) {
        return absl::Status(absl::StatusCode::kInternal,
                            "Unable to set device name.");
    };

    return absl::OkStatus();
}

void RequestForwarder::advertise() {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};
    int rc;

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t*)mDevice.advertisement().device_name().c_str();
    fields.name_len = mDevice.advertisement().device_name().size();
    fields.name_is_complete = 1;

    // Android for some reason expect these to be there, or it will not
    // discover this device.
    struct ble_hs_adv_fields rsp_fields = {0};
    rsp_fields.tx_pwr_lvl = 0xaa;
    rsp_fields.tx_pwr_lvl_is_present = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        derror("error setting advertisement data; rc=%d", rc);
        return;
    }

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        derror("error setting advertisement data; rc=%d", rc);
        return;
    }

    // Begin advertising
    adv_params.conn_mode =
            toNimbleConnectionMode(mDevice.advertisement().connection_mode());
    adv_params.disc_mode =
            toNimbleDiscoveryMode(mDevice.advertisement().discovery_mode());
    rc = ble_gap_adv_start(mAddrType, NULL, BLE_HS_FOREVER, &adv_params,
                           ble_gap_callback, this);
    if (rc != 0) {
        derror("error enabling advertisement; rc=%d", rc);
        return;
    }
}

int RequestForwarder::handleConnect(ble_gap_event* event) {
    // A new connection was established or a connection attempt failed
    dinfo("connection %s; status=%d",
          event->connect.status == 0 ? "established" : "failed",
          event->connect.status);

    if (event->connect.status != 0) {
        // Connection failed; resume advertising
        advertise();
    } else {
        // Notify client..
        mConnections[event->connect.conn_handle] =
                std::make_unique<RemoteConnection>(mClient.get(), mMyId,
                                                   event->connect.conn_handle);
    }

    return 0;
}

int RequestForwarder::handleDisconnect(ble_gap_event* event) {
    dinfo("disconnect; reason=%d", event->disconnect.reason);
    assert(mConnections.count(event->disconnect.conn.conn_handle));
    mConnections.erase(event->disconnect.conn.conn_handle);
    advertise();
    return 0;
}

int RequestForwarder::handleSubscription(ble_gap_event* event) {
    auto chr =
            mProfile.findCharacteristicByHandle(event->subscribe.attr_handle);
    dinfo("subscribe event; cur_notify=%d\n value handle; "
          "val_handle=%d, chr=%p",
          event->subscribe.cur_notify, event->subscribe.attr_handle, chr);

    if (chr == nullptr) {
        derror("chr, not hosted by this handler.");
        return BLE_HS_EUNKNOWN;
    }

    if (!mConnections.count(event->subscribe.conn_handle)) {
        derror("connection handle not connected");
        return BLE_HS_EUNKNOWN;
    }

    bool notify_state = event->subscribe.cur_notify;
    absl::Status status;
    if (notify_state) {
        status = mConnections[event->subscribe.conn_handle]->subscribe(chr);
    } else {
        status = mConnections[event->subscribe.conn_handle]->unsubscribe(chr);
    }
    if (!status.ok()) {
        return BLE_HS_EDISABLED;
    }
    return 0;
}

int RequestForwarder::handleGapEvent(ble_gap_event* event) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            return handleConnect(event);
        case BLE_GAP_EVENT_DISCONNECT:
            return handleDisconnect(event);
        case BLE_GAP_EVENT_ADV_COMPLETE:
            dinfo("adv complete");
            advertise();
            break;
        case BLE_GAP_EVENT_SUBSCRIBE:
            return handleSubscription(event);
        case BLE_GAP_EVENT_MTU:
            dinfo("ignoring.. mtu update event; conn_handle=%d mtu=%d",
                  event->mtu.conn_handle, event->mtu.value);
            break;
    }

    return 0;
}

int RequestForwarder::handleGattAccess(uint16_t connectionHandle,
                                       uint16_t attributeHandle,
                                       struct ble_gatt_access_ctxt* ctxt) {
    if (!mConnections.count(connectionHandle)) {
        derror("Unknown connection %d", connectionHandle);
        return BLE_ATT_ERR_INVALID_HANDLE;
    }

    RemoteConnection* con = mConnections[connectionHandle].get();
    auto chr = mProfile.findCharacteristicByHandle(attributeHandle);
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR: {
            auto response = con->read(chr);
            if (!response.ok())
                return BLE_ATT_ERR_INVALID_HANDLE;
            int rc = os_mbuf_append(ctxt->om, response->data(),
                                    response->size());
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            auto data = collectOmChain(ctxt->om);
            auto response = con->write(chr, data);
            if (!response.ok()) {
                return BLE_ATT_ERR_INVALID_HANDLE;
            }
            return 0;
        }
        // Handle descriptors.
        case BLE_GATT_ACCESS_OP_READ_DSC:
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            break;
        default:
            assert(false);
    };

    return 0;
}

int RequestForwarder::ble_gap_callback(struct ble_gap_event* event, void* arg) {
    return reinterpret_cast<RequestForwarder*>(arg)->handleGapEvent(event);
}

int RequestForwarder::ble_gatt_callback(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt* ctxt,
                                        void* arg) {
    return reinterpret_cast<RequestForwarder*>(arg)->handleGattAccess(
            conn_handle, attr_handle, ctxt);
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
