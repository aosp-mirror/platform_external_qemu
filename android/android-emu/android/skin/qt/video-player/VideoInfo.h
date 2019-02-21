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

// Portions of code is from a tutorial by dranger at gmail dot com:
//   http://dranger.com/ffmpeg
// licensed under the Creative Commons Attribution-Share Alike 2.5 License.

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

#include "android/recording/video/player/FrameQueue.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"

namespace android {
namespace videoplayer {

// Class to grab any useful metadata from a video. Also grabs the first frame
// of a video file for displaying to a widget.
class VideoInfo : public QObject {
    Q_OBJECT

public:
    VideoInfo(VideoPlayerWidget* widget, std::string videoFile);
    virtual ~VideoInfo();

    int getDurationSecs();
    void show();

private:
    void initialize();
    static void getDestinationSize(AVCodecContext* c,
                                   VideoPlayerWidget* widget,
                                   int* pWidth,
                                   int* pHeight);
    static int calculateDurationSecs(AVFormatContext* f);

private:
    std::string mVideoFile;
    VideoPlayerWidget* mWidget = nullptr;
    Frame mPreviewFrame;
    int mDurationSecs;

signals:
    void updateWidget();
};

}  // namespace videoplayer
}  // namespace android
