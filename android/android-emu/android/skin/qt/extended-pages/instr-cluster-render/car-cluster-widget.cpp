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

static constexpr int FRAME_WIDTH = 1280;
static constexpr int FRAME_HEIGHT = 720;

static constexpr int PIPE_START = 1;
static constexpr int PIPE_STOP = 2;
static constexpr int PIPE_CONTINUE = 3;

static CarClusterWidget* instance;

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent),
      mWorkerThread([this](CarClusterWidget::FrameInfo &&frameInfo) {
          return workerProcessFrame(frameInfo);
      }) {

      instance = this;

      set_car_cluster_call_back(processFrame);

      WelsCreateDecoder(&mDecoder);

      mDecodeParams = {nullptr};
      mDecodeParams.sVideoProperty.size = sizeof(mDecodeParams.sVideoProperty);
      mDecodeParams.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
      mDecoder->Initialize(&mDecodeParams);

      mCtx = sws_getContext(FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_YUV420P,
                     FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_RGB32, SWS_BICUBIC,
                     NULL, NULL, NULL);

      mRgbData = new uint8_t[4 * FRAME_WIDTH * FRAME_HEIGHT];

      connect(this, SIGNAL(sendImage(QImage)), this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
      mWorkerThread.start();
}

CarClusterWidget::~CarClusterWidget() {
    mWorkerThread.enqueue({});
    mWorkerThread.join();
    mDecoder->Uninitialize();
    WelsDestroyDecoder(mDecoder);
    delete[] mRgbData;
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

void CarClusterWidget::processFrame(const char* buf, int bufSize) {
    instance->mWorkerThread.enqueue({bufSize, std::vector<char>(buf, buf + bufSize)});
}

WorkerProcessingResult CarClusterWidget::workerProcessFrame(const FrameInfo& frameInfo) {
    if (!frameInfo.size) {
        return WorkerProcessingResult::Stop;
    }
    uint8_t* yuvData[3];
    memset(&mBufferInfo, 0, sizeof(SBufferInfo));
    const uint8_t* currFrame = reinterpret_cast<const uint8_t*>(frameInfo.data.data());
    if (mDecoder->DecodeFrameNoDelay(&currFrame[0], frameInfo.size, yuvData, &mBufferInfo) != 0) {
        // received a bad frame, we can just continue for now
        return WorkerProcessingResult::Continue;
    }

    int ht = mBufferInfo.UsrData.sSystemBuffer.iHeight;
    int wd = mBufferInfo.UsrData.sSystemBuffer.iWidth;

    int stride[3] = {mBufferInfo.UsrData.sSystemBuffer.iStride[0],
                                 mBufferInfo.UsrData.sSystemBuffer.iStride[1],
                                 mBufferInfo.UsrData.sSystemBuffer.iStride[1]};
    int rgbStride[1] = { 4*wd };
    sws_scale(mCtx, yuvData, stride, 0, ht, &mRgbData, rgbStride);
    emit sendImage(QImage(mRgbData, wd, ht, QImage::Format_RGB32).copy());
    return WorkerProcessingResult::Continue;
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

void CarClusterWidget::sendCarClusterMsg(int flag) {
      char msg[1] = {static_cast<char>(flag)};
      android_send_car_cluster_data(msg, 1);
}
