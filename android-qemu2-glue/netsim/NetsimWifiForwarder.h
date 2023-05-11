/* Copyright 2023 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include <memory>

#include "aemu/base/Compiler.h"
#include "android-qemu2-glue/emulation/WifiService.h"
#include "android/network/MacAddress.h"

namespace android {
namespace qemu2 {

class NetsimWifiForwarder : public WifiService {
public:
    NetsimWifiForwarder(WifiService::OnReceiveCallback onRecv,
                        WifiService::CanReceiveCallback canReceive);
    ~NetsimWifiForwarder();
    bool init() override;
    int send(const android::base::IOVector& iov) override;
    int recv(android::base::IOVector& iov) override;
    void stop() override;
    // Not used in netsim wifi grpc connection
    NICState* getNic() override { return nullptr; }
#ifndef LIBSLIRP
    android::network::MacAddress getStaMacAddr(const char* ssid) override {
        return android::network::MacAddress();
    };
#endif

private:
    WifiService::OnReceiveCallback mOnRecv;
    WifiService::CanReceiveCallback mCanReceive;

    DISALLOW_COPY_AND_ASSIGN(NetsimWifiForwarder);
};
}  // namespace qemu2
}  // namespace android