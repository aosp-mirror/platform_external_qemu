
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
#include <memory>                           // for allocator
#include <mutex>                            // for lock_guard, mutex
#include <string>                           // for string, operator==

#include "emulator/webrtc/capture/GrpcAudioSource.h"  // for GrpcAudioSource
#include "emulator/webrtc/capture/GrpcVideoSource.h"  // for GrpcVideoSource

namespace emulator {
namespace webrtc {

MediaSourceLibrary::MediaSourceLibrary(
        EmulatorGrpcClient* client,
        ::webrtc::PeerConnectionFactoryInterface* peerConnectionFactory)
    : mClient(client),
      mPeerConnectionFactory(peerConnectionFactory),
      mAudioSource(mClient) {}


GrpcRefVideoSource MediaSourceLibrary::getVideoSource(int displayId) {
    const std::lock_guard<std::mutex> lock(mAccess);
    auto it = mVideoSources.find(displayId);
    if (it != mVideoSources.end()) {
        return it->second.get();
    }

    // TODO(jansene): Actually support multi display..
    mVideoSources[displayId] = std::make_unique<GrpcVideoMediaSource>(mClient);
    return mVideoSources[displayId].get();
}


}  // namespace webrtc
}  // namespace emulator
