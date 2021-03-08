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

#include <QImage>        // for QImage
#include <QPaintDevice>  // for QPaintDevice
#include <QPainter>      // for QPainter
#include <QRect>         // for QRect

class QPaintEvent;
class QWidget;

namespace android {
namespace videoplayer {

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent), mBuffer(nullptr), mBufferLen(0) {}

VideoPlayerWidget::~VideoPlayerWidget() {}

<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
void VideoPlayerWidget::syncRenderTargetSize(
        float sampleAspectRatio,
        int video_width,
        int video_height,
        int* resultRenderTargetWidth,
        int* resultRenderTargetHeight) {

    int h = height();
    int w = ((int)(h * sampleAspectRatio)) & -3;
    if (w > width()) {
        w = width();
        h = ((int)(w / sampleAspectRatio)) & -3;
    }

    int x = (width() - w) / 2;
    int y = (height() - h) / 2;

    if (width() != w || height() != h) {
        move(x, y);
        setFixedSize(w, h);
=======
void VideoPlayerWidget::getRenderTargetSize(
        float sampleAspectRatio,
        int video_width,
        int video_height,
        int* resultRenderTargetWidth,
        int* resultRenderTargetHeight) {

    int h = height();
    int w = ((int)(h * sampleAspectRatio)) & -3;
    if (w > width()) {
        w = width();
        h = ((int)(w / sampleAspectRatio)) & -3;
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)
    }

    *resultRenderTargetWidth = w;
    *resultRenderTargetHeight = h;
}

void VideoPlayerWidget::paintEvent(QPaintEvent* e) {
    QPainter painter(this);

    if (mBuffer != nullptr && mBufferLen > 0) {
        QImage image = QImage::fromData(mBuffer, mBufferLen, "PPM");
        QRect rect = image.rect();
        QRect destinationRect(0, 0, painter.device()->width(),
                      painter.device()->height());
        rect.moveCenter(destinationRect.center());
        painter.drawImage(rect.topLeft(), image);
    }
}

}  // namespace videoplayer
}  // namespace android
