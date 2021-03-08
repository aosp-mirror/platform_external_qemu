
// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/clipboard/Clipboard.h"

#include "android/base/synchronization/Lock.h"  // for AutoLock, StaticLock

namespace android {
namespace emulation {
namespace control {

Clipboard* sClipboard = nullptr;
android::base::StaticLock sLock;

Clipboard* Clipboard::getClipboard(const QAndroidClipboardAgent* clipAgent) {
    // We do not expect this method to be called frequently (usually only once
    // during emulator startup).
    android::base::AutoLock autoLock(sLock);
    if (!sClipboard) {
        sClipboard = new Clipboard(clipAgent);
    }
    return sClipboard;
}

Clipboard::Clipboard(const QAndroidClipboardAgent* clipAgent)
    : mClipAgent(clipAgent) {
    mClipAgent->setEnabled(true);
    mClipAgent->registerGuestClipboardCallback(
            &Clipboard::guestClipboardCallback, this);
}

void Clipboard::guestClipboardCallback(void* context,
                                       const uint8_t* data,
                                       size_t size) {
    auto self = static_cast<Clipboard*>(context);
    android::base::AutoLock lock(self->mContentsLock);
    self->mContents.assign((char*)data, size);
    self->mEventWaiter.newEvent();
}

void Clipboard::sendContents(const std::string& clipdata) {
    android::base::AutoLock lock(mContentsLock);
    mContents.assign(clipdata);
    mClipAgent->setGuestClipboardContents((uint8_t*)(clipdata.c_str()),
                                          clipdata.size());
}

std::string Clipboard::getCurrentContents() {
    return mContents;
}

EventWaiter* Clipboard::eventWaiter() {
    return &mEventWaiter;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
