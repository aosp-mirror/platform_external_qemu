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

extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
}

class CarClusterWidget : public QWidget
{
    Q_OBJECT

public:
    CarClusterWidget(QWidget* parent = 0);
    ~CarClusterWidget();

    void updatePixmap(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event);

private:
    static void sendCarClusterMsg(uint8_t flag);
    QPixmap mPixmap;
};
