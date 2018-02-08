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

#include "android/skin/qt/video-player/VideoSeekWidget.h"
#include "QtGui/qimage.h"
#include "QtGui/qpainter.h"

namespace android {
namespace videoplayer {

VideoSeekWidget::VideoSeekWidget(QWidget* parent)
    : QWidget(parent) {}

VideoSeekWidget::~VideoSeekWidget() {}

void VideoSeekWidget::paintEvent(QPaintEvent* e) {
    QPainter painter(this);

    painter.setPen(QPen(Qt::red, this->height(), Qt::SolidLine, Qt::SquareCap));
    painter.drawLine(0, 0, mPercentage * this->width(), 0);
}

}  // namespace videoplayer
}  // namespace android

