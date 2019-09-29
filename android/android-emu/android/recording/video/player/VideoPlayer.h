// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <atomic>  // for atomic
#include <memory>  // for unique_ptr
#include <string>  // for string

class VideoPlayerRenderTarget;
namespace offworld {
class DatasetInfo;
}  // namespace offworld

namespace android {
namespace videoplayer {
class VideoPlayerNotifier;

struct PlayConfig {
    // absolute timestamp from the beginning of the video, measured in seconds
    std::atomic<double> seekTimestamp;
    // play in looping mode or not
    std::atomic<bool> looping;

    PlayConfig(double timestamp = 0, bool looping_ = false)
        : seekTimestamp(timestamp), looping(looping_) {}

    // assignment operator because std::atomic deletes it
    PlayConfig& operator=(const PlayConfig& other) {
        seekTimestamp.exchange(other.seekTimestamp);
        looping.exchange(other.looping);
        return *this;
    }
};

// public APIs of the video player
class VideoPlayer {
protected:
    VideoPlayer() = default;

public:
    virtual ~VideoPlayer() = default;

public:
    // create a video player instance the input video file, the output widget to
    // display, and the notifier to receive updates
    static std::unique_ptr<VideoPlayer> create(
            std::string videoFile,
            VideoPlayerRenderTarget* widget,
            std::unique_ptr<VideoPlayerNotifier> notifier);

    virtual void start() = 0;
    virtual void start(const PlayConfig& playConfig) = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void pauseAt(double timestamp) = 0;

    virtual bool isRunning() const = 0;
    virtual bool isLooping() const = 0;

    virtual void videoRefresh() = 0;
    virtual void scheduleRefresh(int delayMs) = 0;

    virtual void loadVideoFileWithData(
            const ::offworld::DatasetInfo& datasetInfo) = 0;

    virtual void setErrorStatusAndRecordErrorMessage(std::string errorDetails) = 0;
    virtual bool getErrorStatus() = 0;
    virtual std::string getErrorMessage() = 0;
};

}  // namespace videoplayer
}  // namespace android
