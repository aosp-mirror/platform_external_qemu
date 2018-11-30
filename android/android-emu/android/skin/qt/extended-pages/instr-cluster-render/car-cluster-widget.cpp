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
#include "android/car-cluster.h"

#include <math.h>
#include <fstream>
#include <iostream>

#include <QPainter>

static constexpr uint8_t PIPE_START = 1;
static constexpr uint8_t PIPE_STOP = 2;
static constexpr uint8_t PIPE_CONTINUE = 3;

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent) {
      set_car_cluster_call_back(processFrame);
}

void CarClusterWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (mPixmap.isNull()) {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Failed to get image, "
                            "possibly due to failed connection, bad frame, etc."));
    } else {
        painter.drawPixmap(rect(), mPixmap);
    }
}

void CarClusterWidget::processFrame(const uint8_t* frame, int frameSize) {
    // TODO: decode input frame to form image & render it
    std::cout << "current frame size: " << frameSize << std::endl;
    sendCarClusterMsg(PIPE_CONTINUE);
    return;
}

void CarClusterWidget::showEvent(QShowEvent* event) {
    sendCarClusterMsg(PIPE_START);
}

void CarClusterWidget::hideEvent(QHideEvent* event) {
    sendCarClusterMsg(PIPE_STOP);
}

void CarClusterWidget::updatePixmap(const QImage& image) {
    mPixmap.convertFromImage(image);
    if (mPixmap.isNull()) {
        return;
    }
    mPixmap = mPixmap.scaled(size());
    repaint();
}

void CarClusterWidget::sendCarClusterMsg(uint8_t flag) {
      uint8_t msg[1] = {flag};
      android_send_car_cluster_data(msg, 1);
}
