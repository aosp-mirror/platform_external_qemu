/* Copyright (C) 2019 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.h"

#include "android/skin/winsys.h"

#include <math.h>
#include <fstream>
#include <iostream>
#include "android/car-cluster.h"

using android::base::AutoLock;
using android::base::System;
using android::base::WorkerProcessingResult;

// Constants shared with Android in NetworkedVirtualDisplay.java
static constexpr uint8_t PIPE_START = 1;
static constexpr uint8_t PIPE_STOP = 2;

static constexpr int FRAME_WIDTH = 1280;
static constexpr int FRAME_HEIGHT = 720;

static constexpr int MSG_START = 1;
static constexpr int MSG_STOP = 2;
static constexpr int MSG_RESTART = 3;

static CarClusterConnector* instance;

CarClusterConnector::CarClusterConnector(CarClusterWindow* carClusterWindow)
    : mCarClusterWindow(carClusterWindow),
      mWorkerThread([this](CarClusterConnector::FrameInfo&& frameInfo) {
          return workerProcessFrame(frameInfo);
      }),
      mCarClusterStartMsgThread([this] {
          while (true) {
              int msg;
              mRefreshMsg.tryReceive(&msg);
              if (msg == MSG_STOP) {
                  break;
              } else if (msg == MSG_START) {
                  AutoLock lock(mCarClusterStartLock);
                  sendCarClusterMsg(PIPE_START);

                  mCarClusterStartCV.timedWait(&mCarClusterStartLock,
                                               nextRefreshAbsolute());
              } else if (msg == MSG_RESTART) {
                  AutoLock lock(mCarClusterStartLock);
                  sendCarClusterMsg(PIPE_STOP);

                  mCarClusterStartCV.timedWait(&mCarClusterStartLock,
                                               nextRefreshAbsolute());
                  sendCarClusterMsg(PIPE_START);
                  break;
              }
          }
      }) {
    instance = this;
    avcodec_register_all();

    mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    mCodecCtx = avcodec_alloc_context3(mCodec);
    avcodec_open2(mCodecCtx, mCodec, 0);
    mFrame = av_frame_alloc();

    mCtx = sws_getContext(FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_YUV420P,
                          FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_RGB32,
                          SWS_BICUBIC, NULL, NULL, NULL);

    mRgbData = new uint8_t[4 * FRAME_WIDTH * FRAME_HEIGHT];

    mWorkerThread.start();
    set_car_cluster_call_back(processFrame);
}

CarClusterConnector::~CarClusterConnector() {
    // Send message to worker thread to stop processing
    mWorkerThread.enqueue({});
    mWorkerThread.join();
    stopSendingStartRequest();
    sendCarClusterMsg(PIPE_STOP);

    av_free(mFrame);
    avcodec_close(mCodecCtx);
    avcodec_free_context(&mCodecCtx);

    sws_freeContext(mCtx);

    delete[] mRgbData;
}

void CarClusterConnector::processFrame(const uint8_t* frame, int frameSize) {
    instance->mWorkerThread.enqueue(
            {frameSize, std::vector<uint8_t>(frame, frame + frameSize)});
}

WorkerProcessingResult CarClusterConnector::workerProcessFrame(
        FrameInfo& frameInfo) {
    if (!frameInfo.size) {
        return WorkerProcessingResult::Stop;
    }
    int rgbStride[1] = {4 * FRAME_WIDTH};

    AVPacket packet;

    av_init_packet(&packet);

    packet.data = frameInfo.frameData.data();

    packet.size = (int)frameInfo.size;

    int frameFinished = 0;

    // TODO: Find better way to silence ffmpeg warning on first packets
    av_log_set_level(AV_LOG_FATAL);

    int nres =
            avcodec_decode_video2(mCodecCtx, mFrame, &frameFinished, &packet);

    av_log_set_level(AV_LOG_INFO);

    auto showFunc = [](void* data) {
        CarClusterWindow* window =
            static_cast<CarClusterWindow*>(data);
        if (!window->isVisible() && !window->isDismissed()) {
            // Open the Cluster window, stop the request thread,
            // Restart the pipe to get first frame
            window->show();
        }
    };

    skin_winsys_run_ui_update(
        showFunc, mCarClusterWindow, true /* wait for result */);

    mRefreshMsg.trySend(MSG_RESTART);

    if (frameFinished > 0) {
        sws_scale(mCtx, mFrame->extended_data, mFrame->linesize, 0,
                  FRAME_HEIGHT, &mRgbData, rgbStride);
        if (mCarClusterWindow) {
            auto pixmapUpdateFunc = [](void* data) {
                CarClusterConnector* connector =
                    static_cast<CarClusterConnector*>(data);
                CarClusterWindow* window = connector->mCarClusterWindow;
                uint8_t* rgbData = connector->mRgbData;
                auto widget = window->findChild<CarClusterWidget*>(
                        "render_window");
                widget->updatePixmap(QImage(
                        rgbData, FRAME_WIDTH, FRAME_HEIGHT, QImage::Format_RGB32));
            };
            skin_winsys_run_ui_update(
                pixmapUpdateFunc, this, false /* async */);
        }
    }

    av_free_packet(&packet);

    return WorkerProcessingResult::Continue;
}

void CarClusterConnector::sendCarClusterMsg(uint8_t flag) {
    uint8_t msg[1] = {flag};
    android_send_car_cluster_data(msg, 1);
}

void CarClusterConnector::startSendingStartRequest() {
    sendCarClusterMsg(PIPE_STOP);
    mRefreshMsg.trySend(MSG_START);
    mCarClusterStartMsgThread.start();
}

void CarClusterConnector::stopSendingStartRequest() {
    mRefreshMsg.trySend(MSG_STOP);
    mCarClusterStartCV.signal();
    mCarClusterStartMsgThread.wait();
}

System::Duration CarClusterConnector::nextRefreshAbsolute() {
    return System::get()->getUnixTimeUs() + REFRESH_INTERVEL;
}
