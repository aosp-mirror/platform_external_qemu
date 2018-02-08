// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/video-player/VideoSeek.h"
#include "android/skin/qt/video-player/VideoSeekWidget.h"

namespace android {
namespace videoplayer {

VideoSeek::VideoSeek() {
    mTimer.setSingleShot(false);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &VideoSeek::repaintSeekWidget);
}

void VideoSeek::setVideoSeekWidget(VideoSeekWidget* widget) {
    mVideoSeekWidget = widget;
}

void VideoSeek::setDurationSecs(int secs) {
    mDurationSecs = secs;
}

void VideoSeek::start() {
    mElapsedTimer.start();
    mTimer.start(1000/REFRESH_RATE);
}

void VideoSeek::stop() {
    mTimer.stop();
    mVideoSeekWidget->setPercentage(0.0);
    mVideoSeekWidget->repaint();
}

void VideoSeek::repaintSeekWidget() {
    if (mDurationSecs <= 0 || !mVideoSeekWidget) {
        return;
    }

    float percentage = (float)mElapsedTimer.elapsed() / (mDurationSecs * 1000);
    if (percentage > 1.0) {
        percentage = 1.0;
        stop();
    }
    mVideoSeekWidget->setPercentage(percentage);
    mVideoSeekWidget->repaint();
}

}  // namespace videoplayer
}  // namespace android


