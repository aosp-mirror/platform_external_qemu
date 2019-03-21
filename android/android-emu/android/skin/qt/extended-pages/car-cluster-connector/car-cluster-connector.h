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
#pragma once
#include <QImage>
#include <QPixmap>
#include <QWidget>

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/WorkerThread.h"
#include "android/skin/qt/car-cluster-window.h"
#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libswscale/swscale.h"
}

// This class is used to build video connection between emulator and Android.
// And to fire the Car Cluster Window. When the emulator starts, this
// CarClusterConnector will send connect request to Android. If there is any
// response, means the Android has car cluster service, then the Car cluster
// window will show and the connection will be rebuild to get the first frame
// which is important to decode the video. When the emulator start with saved
// stated, Android won't send first frame automatically.
class CarClusterConnector {
public:
    CarClusterConnector(CarClusterWindow* carClusterWindow);
    ~CarClusterConnector();

    static void processFrame(const uint8_t* frame, int frameSize);
    void startSendingStartRequest();

private:
    CarClusterWindow* mCarClusterWindow;
    CarClusterWidget* mCarClusterWidget;

    static void sendCarClusterMsg(uint8_t flag);
    struct FrameInfo {
        int size;
        std::vector<uint8_t> frameData;
    };

    struct ResendInfo {
        int64_t time;
        uint8_t pipe_msg;
    };

    android::base::WorkerThread<FrameInfo> mWorkerThread;
    android::base::WorkerProcessingResult workerProcessFrame(
            FrameInfo& frameInfo);

    android::base::FunctorThread mCarClusterStartMsgThread;
    android::base::ConditionVariable mCarClusterStartCV;
    android::base::Lock mCarClusterStartLock;
    android::base::MessageChannel<int, 2> mRefreshMsg;

    void stopSendingStartRequest();

    QPixmap mPixmap;

    AVCodec* mCodec;
    AVCodecContext* mCodecCtx;
    AVFrame* mFrame;

    SwsContext* mCtx;
    uint8_t* mRgbData;

    android::base::System::Duration nextRefreshAbsolute();
    const int64_t REFRESH_INTERVEL = 1000000LL;
};