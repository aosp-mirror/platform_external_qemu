// Copyright (C) 2017 The Android Open Source Project
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

constexpr int kMinVideoBitrate = 100 * 1000;        // bps
constexpr int kMaxVideoBitrate = 25 * 1000 * 1000;  // bps
constexpr int kMaxTimeLimit = 3 * 60;               // seconds

// Spacing between intra frames
constexpr int kIntraSpacing = 12;
// The FPS we are recording at.
constexpr int kFPS = 24;  // fps
// The audio bitrate
constexpr int kAudioBitrate = 64 * 1000;  // bps
// The audio sample rate (44.1 kHz is the qemu audio sample rate)
constexpr int kAudioSampleRate = 44100;  // Hz
// The source number of samples per audio frame
// In Qemu, 512 samples is fixed
constexpr int kSrcNumSamples = 512;
// We're always encoding audio in stereo format
constexpr int kNumAudioChannels = 2;

constexpr int kDefaultVideoBitrate = 4 * 1000 * 1000;  // bps
constexpr int kDefaultTimeLimit = kMaxTimeLimit;

constexpr char kContainerFormat[] = "webm";
