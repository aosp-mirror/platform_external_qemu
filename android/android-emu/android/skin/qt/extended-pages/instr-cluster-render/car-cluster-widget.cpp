// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.h"

#include <math.h>

#include <QPainter>

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent) {
}

void CarClusterWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    if (mPixmap.isNull()) {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Failed to get image, "
                            "possibly due to failed connection, bad frame, etc"));
    } else {
        painter.drawPixmap(rect(), mPixmap);
    }
}

void CarClusterWidget::showEvent(QShowEvent* event) {
    mThread = new CarClusterRenderThread();
    connect(mThread, SIGNAL(renderedImage(QImage)),
            this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    connect(this, SIGNAL(destroyed()), mThread, SLOT(quit()));
    mThread->start();
}

void CarClusterWidget::hideEvent(QHideEvent* event) {
    mThread->stopThread();
    // Make sure previous thread gets destroyed
    mThread->wait();
}

void CarClusterWidget::updatePixmap(const QImage& image) {
    mPixmap.convertFromImage(image);
    if (mPixmap.isNull()) {
        return;
    }
    mPixmap = mPixmap.scaled(size());
    repaint();
}
