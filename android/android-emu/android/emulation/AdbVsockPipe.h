// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/AdbTypes.h"
#include "android/emulation/AndroidPipe.h"

#include <memory>

namespace android {
namespace emulation {

class AdbVsockPipe : public AndroidPipe {
public:
    AdbVsockPipe();

    class Service : public AdbGuestAgent {
    public:
        Service(AdbHostAgent* hostAgent);

        void onHostConnection(android::base::ScopedSocket&& socket,
                              AdbPortType portType) override;

        void resetActiveGuestPipeConnection() override;
    };

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int count) override;
    int onGuestSend(const AndroidPipeBuffer* buffers,
                    int count, void** newPipePtr) override;

    void onGuestWantWakeOn(int flags) override;
    void onSave(android::base::Stream* stream) override;
};

}  // namespace emulation
}  // namespace android
