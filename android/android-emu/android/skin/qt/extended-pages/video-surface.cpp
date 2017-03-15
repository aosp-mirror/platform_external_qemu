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

#include "android/skin/qt/extended-pages/video-surface.h"

#include <QImage>
#include <QSize>
#include <QVideoSurfaceFormat>

VideoSurface::VideoSurface(QWidget* display, QWidget* parent) :
    QAbstractVideoSurface(parent),
    mDisplay(display) {
}
/*
VideoSurface::~VideoSurface() {
}
*/

QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const {
    Q_UNUSED(handleType);

    // Return the formats supported
    return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_YUV420P;
}

bool VideoSurface::present(const QVideoFrame& frame) {
    // Don't render it if the frame is in an unsupported format.
    if (frame.pixelFormat() != QVideoFrame::Format_YUV420P) {
        fprintf(stderr, "Video has unsupported format(%d)\n", (int)frame.pixelFormat());
        return false;
    }
}

bool VideoSurface::start(const QVideoSurfaceFormat &format)
{
    const QImage::Format imageFormat =
        QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();

    if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) {
        mImageFormat = imageFormat;
        mImageSize = size;
        mSourceRect = format.viewport();

        QAbstractVideoSurface::start(format);

        mDisplay->updateGeometry();
        updateVideoRect();

        return true;
    } else {
        return false;
    }
}

void VideoSurface::updateVideoRect()
{
    QSize size = surfaceFormat().sizeHint();
    size.scale(mDisplay->size().boundedTo(size), Qt::KeepAspectRatio);

    mTargetRect = QRect(QPoint(0, 0), size);
    mTargetRect.moveCenter(mDisplay->rect().center());
}

void VideoSurface::paint(QPainter *painter)
{
    if (mCurrentFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        const QTransform oldTransform = painter->transform();

        if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
            painter->scale(1, -1);
            painter->translate(0, -mDisplay->height());
        }

        QImage image(
                mCurrentFrame.bits(),
                mCurrentFrame.width(),
                mCurrentFrame.height(),
                mCurrentFrame.bytesPerLine(),
                mImageFormat);

        painter->drawImage(mTargetRect, image, mSourceRect);

        painter->setTransform(oldTransform);

        mCurrentFrame.unmap();
    }
}

void VideoSurface::stop()
{
    mCurrentFrame = QVideoFrame();
    mTargetRect = QRect();

    QAbstractVideoSurface::stop();

    mDisplay->update();
}
