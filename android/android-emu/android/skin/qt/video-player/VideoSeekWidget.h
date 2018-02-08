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

#include <QtWidgets/QWidget>

namespace android {
namespace videoplayer {

class VideoSeekWidget : public QWidget {
    Q_OBJECT

public:
    VideoSeekWidget(QWidget* parent = nullptr);
    ~VideoSeekWidget();

    // |percent| should be between 0 and 1
    void setPercentage(float percent) {
        mPercentage = percent;
    }

private:
    void paintEvent(QPaintEvent*);

private:
    float mPercentage = 0;

public:
    static constexpr int VIDEO_SEEKBAR_HEIGHT = 4;
};

}  // namespace videoplayer
}  // namespace android

