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

#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

namespace android {
namespace videoplayer {

class VideoSeekWidget;

// public APIs of the video player
class VideoSeek : public QObject {
    Q_OBJECT

public:
    VideoSeek();

    virtual ~VideoSeek() = default;

public:
    void setVideoSeekWidget(VideoSeekWidget* widget);
    void setDurationSecs(int secs);
    void start();
    void stop();

private slots:
    void repaintSeekWidget();

private:
    // number of times to call repaint() per second
    static constexpr int REFRESH_RATE = 20;
    int mDurationSecs = 0;
    VideoSeekWidget* mVideoSeekWidget = nullptr;
    QTimer mTimer;
    QElapsedTimer mElapsedTimer;
};

}  // namespace videoplayer
}  // namespace android

