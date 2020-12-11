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

#include "android/emulation/AdbVsockPipe.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#include <sys/socket.h>
#endif

namespace android {
namespace emulation {

AdbVsockPipe::Service::Service(AdbHostAgent* hostAgent) {
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             AdbPortType portType) {
}

void AdbVsockPipe::Service::resetActiveGuestPipeConnection() {
}

AdbVsockPipe::AdbVsockPipe() : AndroidPipe(nullptr, nullptr) {

}

void AdbVsockPipe::onGuestClose(PipeCloseReason reason) {
}

unsigned AdbVsockPipe::onGuestPoll() const {
    return 0;
}

int AdbVsockPipe::onGuestRecv(AndroidPipeBuffer* buffers, int count) {
    return 0;
}

int AdbVsockPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int count, void** newPipePtr) {
    return 0;
}

void AdbVsockPipe::onGuestWantWakeOn(int flags) {
}

void AdbVsockPipe::onSave(android::base::Stream* stream) {
}

}  // namespace emulation
}  // namespace android
