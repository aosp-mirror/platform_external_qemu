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
#pragma once
#include "emulator/webrtc/capture/GrpcAudioSource.h"
#include "emulator/webrtc/capture/GrpcVideoSource.h"

namespace webrtc {
class PeerConnectionFactoryInterface;
}  // namespace webrtc

namespace emulator {
namespace webrtc {

// A MediaSourceLibrary keeps track of all available gRPC based media sources. A
// media source will start producing data as soon as it is checked out.
class MediaSourceLibrary {
public:
    MediaSourceLibrary(
            EmulatorGrpcClient* client,
            ::webrtc::PeerConnectionFactoryInterface* peerConnectionFactory);

    // This is a refcounted source, do not delete it.
    GrpcRefAudioSource getAudioSource() { return &mAudioSource; }

    // This is a refcounted source, do not delete it.
    GrpcRefVideoSource getVideoSource(int displayId);

private:
    EmulatorGrpcClient* mClient;
    ::webrtc::PeerConnectionFactoryInterface* mPeerConnectionFactory;
    std::mutex mAccess;
    GrpcAudioMediaSource mAudioSource;
    std::unordered_map<int, std::unique_ptr<GrpcVideoMediaSource>>
            mVideoSources;
};
}  // namespace webrtc
}  // namespace emulator
