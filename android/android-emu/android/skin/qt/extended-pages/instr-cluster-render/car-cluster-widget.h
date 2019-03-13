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
#pragma once
#include <QPixmap>
#include <QWidget>
#include <QImage>

#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/WorkerThread.h"

extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
}

using android::base::System;
using android::base::FunctorThread;

class CarClusterWidget : public QWidget
{
    Q_OBJECT

public:
    CarClusterWidget(QWidget* parent = 0);
    ~CarClusterWidget();

    static void processFrame(const uint8_t* frame, int frameSize);

signals:
    void sendImage(const QImage &image);

protected:
    void paintEvent(QPaintEvent* event);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private slots:
    void updatePixmap(const QImage& image);

private:
    static void sendCarClusterMsg(uint8_t flag);

    struct FrameInfo {
        int size;
        std::vector<uint8_t> frameData;
    };

    android::base::WorkerThread<FrameInfo> mWorkerThread;
    android::base::WorkerProcessingResult workerProcessFrame(FrameInfo& frameInfo);

    QPixmap mPixmap;

    AVCodec* mCodec;
    AVCodecContext* mCodecCtx;
    AVFrame* mFrame;

    SwsContext* mCtx;

    FunctorThread mCarClusterStartMsgThread;
    std::atomic<bool> mCarClusterStartFlag;

    uint8_t* mRgbData;
};
