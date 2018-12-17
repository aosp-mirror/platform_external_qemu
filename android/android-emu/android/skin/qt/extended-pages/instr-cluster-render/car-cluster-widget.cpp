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

using android::base::WorkerProcessingResult;

static constexpr uint8_t PIPE_START = 1;
static constexpr uint8_t PIPE_STOP = 2;

static constexpr int FRAME_WIDTH = 1280;
static constexpr int FRAME_HEIGHT = 720;

static CarClusterWidget* instance;

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent),
      mWorkerThread([this](CarClusterWidget::FrameInfo &&frameInfo) {
          return workerProcessFrame(frameInfo);
      }) {
      instance = this;

      avcodec_register_all();

      mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
      mCodecCtx = avcodec_alloc_context3(mCodec);
      avcodec_open2(mCodecCtx,mCodec,0);
      mFrame = av_frame_alloc();

      mCtx = sws_getContext(FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_YUV420P,
                     FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_RGB32, SWS_BICUBIC,
                     NULL, NULL, NULL);

      mRgbData = new uint8_t[4 * FRAME_WIDTH * FRAME_HEIGHT];

      connect(this, SIGNAL(sendImage(QImage)),
                this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
      mWorkerThread.start();

      set_car_cluster_call_back(processFrame);
}

CarClusterWidget::~CarClusterWidget() {
    // Send message to worker thread to stop processing
    mWorkerThread.enqueue({});
    mWorkerThread.join();

    av_free(mFrame);
    avcodec_close(mCodecCtx);
    avcodec_free_context(&mCodecCtx);

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

void CarClusterWidget::processFrame(const uint8_t* frame, int frameSize) {
    instance->mWorkerThread.enqueue({frameSize, std::vector<uint8_t>(frame, frame + frameSize)});
}

WorkerProcessingResult CarClusterWidget::workerProcessFrame(FrameInfo& frameInfo) {
    if (!frameInfo.size) {
        return WorkerProcessingResult::Stop;
    }
    int rgbStride[1] = {4 * FRAME_WIDTH};

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = frameInfo.frameData.data();
    packet.size = (int) frameInfo.size;
    int frameFinished = 0;

    // TODO: Find better way to silence ffmpeg warning on first packets
    av_log_set_level(AV_LOG_FATAL);
    int nres = avcodec_decode_video2(mCodecCtx,mFrame,&frameFinished,&packet);
    av_log_set_level(AV_LOG_INFO);

    if (frameFinished > 0) {
        sws_scale(mCtx, mFrame->extended_data, mFrame->linesize,
                    0, FRAME_HEIGHT, &mRgbData, rgbStride);
        emit sendImage(QImage(mRgbData, FRAME_WIDTH, FRAME_HEIGHT, QImage::Format_RGB32));
    }

    av_free_packet(&packet);
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

void CarClusterWidget::sendCarClusterMsg(uint8_t flag) {
      uint8_t msg[1] = {flag};
      android_send_car_cluster_data(msg, 1);
}
