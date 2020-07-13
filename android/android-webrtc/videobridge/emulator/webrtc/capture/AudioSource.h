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
#pragma once

#include <stdio.h>                       // for size_t
#include <mutex>                         // for mutex, lock_guard

#include "api/media_stream_interface.h"  // for MediaSourceInterface::Source...

namespace emulator {
namespace webrtc {

class AudioSource : public ::webrtc::AudioSourceInterface {
public:
    AudioSource() = default;
    void AddSink(::webrtc::AudioTrackSinkInterface* sink) override {
        const std::lock_guard<std::mutex> lock(mMutex);
        mSink = sink;
    }
    void RemoveSink(::webrtc::AudioTrackSinkInterface* sink) override {
        const std::lock_guard<std::mutex> lock(mMutex);
        mSink = nullptr;
    }

    ::webrtc::MediaSourceInterface::SourceState state() const override {
        return ::webrtc::MediaSourceInterface::SourceState::kLive;
    }
    // Called to register state change observers.  Since state is always the
    // same, state change observers are ignored.
    void RegisterObserver(::webrtc::ObserverInterface* observer) override {}
    void UnregisterObserver(::webrtc::ObserverInterface* observer) override {}

    bool remote() const override { return false; }

    void OnData(const void* audio_data,
                int bits_per_sample,
                int sample_rate,
                size_t number_of_channels,
                size_t number_of_frames);

private:
    std::mutex mMutex;
    ::webrtc::AudioTrackSinkInterface* mSink = nullptr;
};
}  // namespace webrtc
}  // namespace emulator
