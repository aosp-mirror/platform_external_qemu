// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <queue>

#include "android/emulation/apacket_utils.h"

namespace android {
namespace emulation {

class AdbProxy {
public:
    /*
     * onGuestRecvData got triggered before guest receives data.
     * Args:
     *   mesg: the message header.
     *   data: the message body.
     *   shouldForwardRecv: an output value to tell if the message should
     *                      be forwarded to the guest as it is.
     *   toSends: an output queue to tell if the proxy sends extra
     *            packages to the host.
     *
     * When overriding this method:
     * If the proxy decides to consume the package itself, a common
     * practice is to set shouldForwardRecv to false, and put its reply
     * into toSends.
     */
    virtual void onGuestRecvData(const android::emulation::amessage* mesg,
                                 const uint8_t* data,
                                 bool* shouldForwardRecv,
                                 std::queue<apacket>* toSends) = 0;
    /*
     * onGuestSendData got triggered after guest sends data and before
     * the data is sent to host socket.
     * Args:
     *   mesg: the message header.
     *   data: the message body.
     *   shouldForwardSend: an output value to tell if the message should
     *                      be forwarded to the host as it is.
     *   extraSends: an output queue to tell if the proxy sends extra
     *               packages to the host.
     *
     * When overriding this method:
     * If the proxy decides to append extra packages, a common practice
     * is to set shouldForwardSend to false, and put all the to-send
     * packages to extraSends.
     * It is advised not to set shouldForwardSend to true while still
     * putting extra packages in extraSends. If there is a need to
     * simultaneously forward a message as well as sending extra messages,
     * one should consider copying the message into extraSends manually,
     * so that you can control the send ordering.
     */
    virtual void onGuestSendData(
            const android::emulation::amessage* mesg,
            const uint8_t* data,
            bool* shouldForwardSend,
            std::queue<emulation::apacket>* extraSends) = 0;
    virtual bool shouldClose() const = 0;
    virtual int32_t guestId() const = 0;
    virtual int32_t originHostId() const = 0;
    virtual int32_t currentHostId() const = 0;
};
}  // namespace emulation
}  // namespace android
