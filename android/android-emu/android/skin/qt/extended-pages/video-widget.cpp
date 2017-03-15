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

#include "android/skin/qt/extended-pages/video-widget.h"

#include <QPaintEvent>
#include <QVideoSurfaceFormat>

VideoWidget::VideoWidget(QWidget *parent) :
        QWidget(parent),
        mSurface(nullptr) {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_PaintOnScreen, true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    mSurface.reset(new VideoSurface(this));
}

QAbstractVideoSurface* VideoWidget::videoSurface() const {
    return mSurface.get();
}

QSize VideoWidget::sizeHint() const
{
    return mSurface->surfaceFormat().sizeHint();
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (mSurface->isActive()) {
        const QRect videoRect = mSurface->videoRect();

        if (!videoRect.contains(event->rect())) {
            QRegion region = event->region();
            region = region.subtracted(videoRect);

            QBrush brush = palette().background();

            foreach (const QRect &rect, region.rects())
                painter.fillRect(rect, brush);
        }

        mSurface->paint(&painter);
    } else {
        painter.fillRect(event->rect(), palette().background());
    }
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    mSurface->updateVideoRect();
}
