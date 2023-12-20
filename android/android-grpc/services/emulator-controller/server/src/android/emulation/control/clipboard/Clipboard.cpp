
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

#include <memory>
#include <vector>

#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "android/utils/debug.h"
#include "google/protobuf/util/message_differencer.h"

namespace android {
namespace emulation {
namespace control {

static std::string peerId(const ::grpc::ServerContextBase* context) {
    std::string flat;
    for (const auto& str : context->auth_context()->GetPeerIdentity()) {
        flat += ":" + std::string(str.data(), str.size());
    }

    flat += ":" + context->peer();
    return flat;
}

class ClipDataEventStreamWriter
    : public BaseEventStreamWriter<ClipData, ClipboardEvent> {
public:
    ClipDataEventStreamWriter(EventChangeSupport<ClipboardEvent>* listener,
                              ::grpc::CallbackServerContext* context)
        : BaseEventStreamWriter<ClipData, ClipboardEvent>(listener),
          mContext(context) {}
    virtual ~ClipDataEventStreamWriter() = default;

    void eventArrived(const ClipboardEvent event) override {
        std::string dest = peerId(mContext);
        const std::lock_guard<std::mutex> lock(mEventLock);
        if (google::protobuf::util::MessageDifferencer::Equals(event.data,
                                                               mLastEvent)) {
            VERBOSE_INFO(keys, "%s is already aware of %s", dest,
                         event.data.ShortDebugString());
            return;
        }

        mLastEvent = event.data;

        if (dest != event.source) {
            VERBOSE_INFO(keys, "Event from: %s for %s (%s)",
                         event.source, dest,
                         event.data.ShortDebugString());

            SimpleServerWriter<ClipData>::Write(event.data);
        } else {
            VERBOSE_INFO(keys, "Dropping update from: %s for %s ",
                         event.source.c_str(), dest.c_str());
        }
    }

private:
    ::grpc::ServerContextBase* mContext;

    ClipData mLastEvent;
    std::mutex mEventLock;
};

Clipboard* sClipboard = nullptr;
std::mutex sLock;

Clipboard* Clipboard::getClipboard(const QAndroidClipboardAgent* clipAgent) {
    // We do not expect this method to be called frequently (usually only once
    // during emulator startup).
    std::lock_guard lock(sLock);
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
    std::lock_guard lock(self->mContentsLock);
    self->mContents.assign((char*)data, size);

    // Note that the source will never match anyone! (peerId always starts with
    // a ':')
    ClipboardEvent event{.source = "android"};
    event.data.set_text(self->mContents);

    VERBOSE_INFO(keys, "Forwarding clipboard event: `%s` to gRPC listeners.",
                 event.data.ShortDebugString().c_str());
    self->fireEvent(event);
}

void Clipboard::sendContents(const std::string& clipdata,
                             const ::grpc::ServerContextBase* from) {
    std::lock_guard lock(mContentsLock);
    mContents.assign(clipdata);
    mClipAgent->setGuestClipboardContents((uint8_t*)(clipdata.c_str()),
                                          clipdata.size());

    ClipboardEvent event{.source = peerId(from)};
    event.data.set_text(clipdata);
    VERBOSE_INFO(
            keys,
            "Forwarding clipboard event: `%s` from `%s` to gRPC listeners.",
            event.data.ShortDebugString().c_str(), event.source.c_str());
    fireEvent(event);
}

std::string Clipboard::getCurrentContents() {
    return mContents;
}

grpc::ServerWriteReactor<ClipData>* Clipboard::eventWriter(
        ::grpc::CallbackServerContext* context) {
    VERBOSE_INFO(keys, "Registering listener with id: %s",
                 peerId(context).c_str());
    // Note that the source will never match anyone! (peerId always starts with
    // a ':')
    ClipboardEvent event{.source = "android"};
    event.data.set_text(getCurrentContents());

    auto stream = new ClipDataEventStreamWriter(this, context);
    stream->eventArrived(event);

    return stream;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
