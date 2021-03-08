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

#include <qnamespace.h>                         // for AlignCenter, black
#include <QPainter>                             // for QPainter

#include "android/base/threads/WorkerThread.h"  // for WorkerProcessingResult

class QImage;
class QPaintEvent;
class QWidget;

using android::base::WorkerProcessingResult;

CarClusterWidget::CarClusterWidget(QWidget* parent) : QWidget(parent) {}

CarClusterWidget::~CarClusterWidget() {}

void CarClusterWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (mPixmap.isNull()) {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Waiting for connection, "
                            "please wait for android to start completely"));
    } else {
        painter.drawPixmap(rect(), mPixmap);
    }
}

void CarClusterWidget::updatePixmap(const QImage& image) {
    mPixmap.convertFromImage(image);
    if (mPixmap.isNull()) {
        return;
    }
    mPixmap = mPixmap.scaled(size());
    repaint();
}
