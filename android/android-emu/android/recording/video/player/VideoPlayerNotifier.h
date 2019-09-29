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

#include <functional>  // for function

namespace android {
namespace videoplayer {

class VideoPlayer;

// Abstract VideoPlayerNotifier class, backing AndroidEmu and Qt impls.
class VideoPlayerNotifier {

public:

    virtual ~VideoPlayerNotifier() = default;

    using WidgetUpdateCallback = std::function<void()>;

    void setVideoPlayer(VideoPlayer* player) { mPlayer = player; }

    void onVideoRefresh();

    virtual void initTimer() = 0;
    virtual void startTimer(int delayMs) = 0;
    virtual void stopTimer() = 0;
    virtual void emitUpdateWidget() = 0;
    virtual void emitVideoFinished() = 0;
    virtual void emitVideoStopped() = 0;

protected:
    VideoPlayer* mPlayer = nullptr;
};

}  // namespace videoplayer
}  // namespace android
