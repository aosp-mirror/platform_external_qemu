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
#include "emulator/webrtc/capture/GrpcAudioSource.h"

#include <grpcpp/grpcpp.h>     // for string
#include <rtc_base/logging.h>  // for RTC_LOG
#include <stdio.h>             // for size_t

#include <algorithm>  // for min
#include <map>        // for multimap
#include <memory>     // for unique_ptr
#include <string>     // for string
#include <thread>     // for thread::id
#include <utility>    // for __unwra...

#include "android/base/StringView.h"                          // for StringView
#include "android/base/files/IniFile.h"                       // for IniFile
#include "android/emulation/control/secure/BasicTokenAuth.h"  // for BasicTo...
#include "emulator_controller.pb.h"                           // for AudioPa...
#include "grpcpp/security/credentials.h"                      // for Insecur...

namespace emulator {
namespace webrtc {

// A plugin that inserts the basic auth headers into a gRPC request
// if needed.
class BasicTokenAuthenticator : public grpc::MetadataCredentialsPlugin {
public:
    BasicTokenAuthenticator(const grpc::string& token)
        : mToken("Bearer " + token) {}
    ~BasicTokenAuthenticator() = default;

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        metadata->insert(std::make_pair(
                android::emulation::control::BasicTokenAuth::DEFAULT_HEADER,
                mToken));
        return grpc::Status::OK;
    }

private:
    grpc::string mToken;
};

using ::android::emulation::control::AudioFormat;
using ::android::emulation::control::AudioPacket;

constexpr int32_t kBitRateHz = 44100;
constexpr int32_t kChannels = 2;
constexpr int32_t kBytesPerSample = 2;

// WebRTC requires each audio packet to be 10ms long.
constexpr int32_t kSamplesPerFrame = kBitRateHz / 100 /*(10ms/1s)*/;
constexpr size_t kBytesPerFrame =
        kBytesPerSample * kSamplesPerFrame * kChannels;

GrpcAudioSource::GrpcAudioSource(std::string discovery_file) {
    if (initializeGrpcStub(discovery_file)) {
        mPartialFrame.reserve(kBytesPerFrame);
        mAudioThread = std::thread([this]() { StreamAudio(); });
    }
}

bool GrpcAudioSource::initializeGrpcStub(std::string discovery_file) {
    android::base::IniFile iniFile(discovery_file);
    iniFile.read();
    if (!iniFile.hasKey("grpc.port") || iniFile.hasKey("grpc.server_cert")) {
        RTC_LOG(LERROR) << "No grpc port, or tls enabled. Audio disabled.";
        return false;
    }
    std::string grpc_address =
            "localhost:" + iniFile.getString("grpc.port", "8554");
    mEmulatorStub = android::emulation::control::EmulatorController::NewStub(
            grpc::CreateChannel(
                    grpc_address,
                    ::grpc::experimental::LocalCredentials(LOCAL_TCP)));

    // Install token authenticator if needed.
    if (iniFile.hasKey("grpc.token")) {
        auto token = iniFile.getString("grpc.token", "");
        auto creds = grpc::MetadataCredentialsFromPlugin(
                std::make_unique<BasicTokenAuthenticator>(token));
        mContext.set_credentials(creds);
    }

    RTC_LOG(INFO) << "Initialized audio.";
    return true;
}

void GrpcAudioSource::StreamAudio() {
    AudioFormat request;
    request.set_format(AudioFormat::AUD_FMT_S16);
    request.set_channels(AudioFormat::Stereo);
    request.set_samplingrate(kBitRateHz);
    std::unique_ptr<grpc::ClientReaderInterface<AudioPacket>> stream =
            mEmulatorStub->streamAudio(&mContext, request);
    if (stream == nullptr) {
        return;
    }
    AudioPacket audio_packet;
    while (stream->Read(&audio_packet)) {
        ConsumeAudioPacket(audio_packet);
    }
}

void GrpcAudioSource::ConsumeAudioPacket(const AudioPacket& audio_packet) {
    size_t bytes_consumed = 0;
    if (!mPartialFrame.empty()) {
        size_t bytes_to_append = std::min(kBytesPerFrame - mPartialFrame.size(),
                                          audio_packet.audio().size());
        mPartialFrame.insert(mPartialFrame.end(), audio_packet.audio().data(),
                             audio_packet.audio().data() + bytes_to_append);
        if (mPartialFrame.size() < kBytesPerFrame) {
            return;
        }
        bytes_consumed += bytes_to_append;
        OnData(&mPartialFrame.front(), kBytesPerSample * 8, kBitRateHz,
               kChannels, kSamplesPerFrame);
    }

    while (bytes_consumed + kBytesPerFrame <= audio_packet.audio().size()) {
        OnData(audio_packet.audio().data() + bytes_consumed,
               kBytesPerSample * 8, kBitRateHz, kChannels, kSamplesPerFrame);
        bytes_consumed += kBytesPerFrame;
    }

    mPartialFrame.assign(
            audio_packet.audio().data() + bytes_consumed,
            audio_packet.audio().data() + audio_packet.audio().size());
}

GrpcAudioSource::~GrpcAudioSource() {
    mContext.TryCancel();
    mCaptureAudio = false;
    mAudioThread.join();
}

}  // namespace webrtc
}  // namespace emulator
