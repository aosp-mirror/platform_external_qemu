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
#include "emulator/webrtc/RtcConnection.h"  // for RtcCo...

#include <api/audio/audio_mixer.h>                           // for Audio...
#include <api/audio_codecs/builtin_audio_decoder_factory.h>  // for Creat...
#include <api/audio_codecs/builtin_audio_encoder_factory.h>  // for Creat...
#include <api/create_peerconnection_factory.h>               // for Creat...
#include <api/make_ref_counted.h>
#include <api/peer_connection_interface.h>                      // for PeerC...
#include <api/scoped_refptr.h>                                  // for scope...
#include <api/task_queue/default_task_queue_factory.h>          // for Creat...
#include <api/task_queue/task_queue_factory.h>                  // for TaskQ...
#include <api/video_codecs/builtin_video_decoder_factory.h>     // for Creat...
#include <api/video_codecs/builtin_video_encoder_factory.h>     // for Creat...
#include <modules/audio_device/include/audio_device.h>          // for Audio...
#include <modules/audio_processing/include/audio_processing.h>  // for Audio...
#include <rtc_base/logging.h>                                   // for RTC_LOG
#include <rtc_base/thread.h>                                    // for Thread
#include <functional>                                           // for __base
#include <memory>                                               // for uniqu...
#include <thread>                                               // for thread

#include "aemu/base/Log.h"                                      // for base
#include "aemu/base/async/Looper.h"                             // for Looper
#include "emulator/webrtc/capture/GoldfishAudioDeviceModule.h"  // for Goldf...

using namespace android::base;

namespace emulator {
namespace webrtc {

RtcConnection::RtcConnection()
    : mTaskFactory(::webrtc::CreateDefaultTaskQueueFactory()),
      mNetwork(rtc::Thread::CreateWithSocketServer()),
      mWorker(rtc::Thread::Create()),
      mSignaling(rtc::Thread::Create()),
      mKeepAlive(
              &mLooper,
              [this]() { return mLooperActive; },
              500) {
    mNetwork->SetName("Sw-Network", nullptr);
    mNetwork->Start();
    mWorker->SetName("Sw-Worker", nullptr);
    mWorker->Start();
    mSignaling->SetName("Sw-Signaling", nullptr);
    mSignaling->Start();
    mConnectionFactory = ::webrtc::CreatePeerConnectionFactory(
            mNetwork.get(), mWorker.get(), mSignaling.get(),
             rtc::scoped_refptr<GoldfishAudioDeviceModule>(new GoldfishAudioDeviceModule()),
            ::webrtc::CreateBuiltinAudioEncoderFactory(),
            ::webrtc::CreateBuiltinAudioDecoderFactory(),
            ::webrtc::CreateBuiltinVideoEncoderFactory(),
            ::webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr /* audio_mixer */, nullptr /* audio_processing */);

    mLibrary = std::make_unique<MediaSourceLibrary>(mConnectionFactory.get());
    mKeepAlive.start();
    mLooperThread = std::make_unique<std::thread>([this]() {
        mLooper.runWithDeadlineMs(Looper::Timeout::kDurationInfinite);
    });
}

RtcConnection::~RtcConnection() {
    // Stop our looper.
    mLooperActive = false;
    mLooper.forceQuit();
    mLooperThread->join();

    // First we need to clean up the connection factory.
    mConnectionFactory = nullptr;

    // And then shut down the WebRTC threads.
    mSignaling->Stop();
    mWorker->Stop();
    mNetwork->Stop();
}

}  // namespace webrtc
}  // namespace emulator
