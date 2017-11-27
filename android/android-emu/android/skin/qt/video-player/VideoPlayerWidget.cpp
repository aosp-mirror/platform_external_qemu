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

#include "android/skin/qt/video-player/VideoPlayerWidget.h"
#include "QtGui/qimage.h"
#include "QtGui/qpainter.h"

namespace android {
namespace videoplayer {

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent), buf(nullptr), len(0) {}

VideoPlayerWidget::~VideoPlayerWidget() {}

void VideoPlayerWidget::resize() {}

void VideoPlayerWidget::paintEvent(QPaintEvent* e) {
    QPainter painter(this);

    if (buf != nullptr && len > 0) {
        QImage i = QImage::fromData(buf, len, "PPM");
        painter.drawImage(QPoint(0, 0), i);
    }
}

} // namespace videoplayer
} // namespace android
