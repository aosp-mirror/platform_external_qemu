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
#pragma once

#include <QAbstractVideoSurface>
#include <QImage>
#include <QPainter>
#include <QWidget>
#include <QVideoFrame>

#include <memory>

class VideoSurface : public QAbstractVideoSurface {
    Q_OBJECT
public:
    explicit VideoSurface(QWidget* display, QWidget* parent = 0);
    ~VideoSurface() = default;

    virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
    // Displays decoded frame to the display.
    bool present(const QVideoFrame& frame);
    bool start(const QVideoSurfaceFormat& format);
    void stop();

    QRect videoRect() const { return mTargetRect; }
    void updateVideoRect();

    void paint(QPainter* painter);

signals:

private slots:

public slots:

public:

private:
    QWidget* mDisplay;
    QImage::Format mImageFormat;
    QRect mSourceRect;
    QRect mTargetRect;
    QSize mImageSize;
    QVideoFrame mCurrentFrame;
};
