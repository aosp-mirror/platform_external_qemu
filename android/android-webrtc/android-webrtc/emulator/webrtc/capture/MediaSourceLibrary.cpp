
// Copyright (C) 2021 The Android Open Source Project
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
#include "emulator/webrtc/capture/MediaSourceLibrary.h"

#include <api/media_stream_interface.h>     // for AudioTrackInter...
#include <api/peer_connection_interface.h>  // for PeerConnectionF...
#include <api/scoped_refptr.h>              // for scoped_refptr
#include <rtc_base/ref_counted_object.h>    // for RefCountedObject
#include <string>                           // for string, operator==

#include "emulator/webrtc/capture/AudioSource.h"  // for InprocessAudioSource
#include "emulator/webrtc/capture/InprocessVideoSource.h"  // for InprocessVideoSource

namespace emulator {
namespace webrtc {

MediaSourceLibrary::MediaSourceLibrary(
        ::webrtc::PeerConnectionFactoryInterface* peerConnectionFactory)
    : mPeerConnectionFactory(peerConnectionFactory), mAudioSource() {}

InprocessRefVideoSource MediaSourceLibrary::getVideoSource(int displayId) {
    const std::lock_guard<std::mutex> lock(mAccess);
    auto it = mVideoSources.find(displayId);
    if (it != mVideoSources.end()) {
        return InprocessRefVideoSource(it->second.get());
    }

    mVideoSources[displayId] =
            std::make_unique<InprocessVideoMediaSource>(displayId);
    return InprocessRefVideoSource(mVideoSources[displayId].get());
}

}  // namespace webrtc
}  // namespace emulator
