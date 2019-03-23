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

#include "android/recording/video/player/VideoPlayerNotifier.h"

#include <QObject>
#include <QTimer>

namespace android {
namespace videoplayer {

class VideoPlayer;
class VideoPlayerWidget;

// Qt related functions for the video player
// to notifier updates to the caller and a timer for the player
// to refresh video displays

class QtVideoPlayerNotifier : public QObject, public VideoPlayerNotifier {
    Q_OBJECT

public:
    QtVideoPlayerNotifier() = default;

    virtual ~QtVideoPlayerNotifier() = default;

    // initialize the Qt timer, must be called from Qt UI thread
    void initTimer() override;

    // start the Qt timer, must be called from Qt UI thread
    void startTimer(int delayMs) override;

    // stop the Qt timer, must be called from Qt UI thread
    void stopTimer() override;

    void emitUpdateWidget() override { emit updateWidget(); }

    void emitVideoFinished() override { emit videoFinished(); }

    void emitVideoStopped() override { emit videoStopped(); }

private:

    QTimer mTimer;

private slots:
    void videoRefreshTimer();

signals:
    void updateWidget();
    void videoFinished();
    void videoStopped();
};

}  // namespace videoplayer
}  // namespace android
