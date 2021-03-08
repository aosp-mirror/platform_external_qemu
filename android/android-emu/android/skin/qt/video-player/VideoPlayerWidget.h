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

#pragma once

<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
#include "android/recording/video/player/VideoPlayerRenderTarget.h"

#include <QtWidgets/QWidget>
=======
#include <qobjectdefs.h>
#include <stddef.h>
#include <QString>
#include <QWidget>

#include "android/recording/video/player/VideoPlayerRenderTarget.h"

class QObject;
class QPaintEvent;
class QWidget;
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)

namespace android {
namespace videoplayer {

class VideoPlayerWidget : public QWidget, public VideoPlayerRenderTarget {
    Q_OBJECT

public:
    VideoPlayerWidget(QWidget* parent = nullptr);
    ~VideoPlayerWidget();

<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
    void syncRenderTargetSize(
        float sampleAspectRatio,
        int video_width,
        int video_height,
        int* resultRenderTargetWidth,
        int* resultRenderTargetHeight) override;

    void setPixelBuffer(const unsigned char* buf, size_t len) override {
=======
    void getRenderTargetSize(float sampleAspectRatio,
                             int video_width,
                             int video_height,
                             int* resultRenderTargetWidth,
                             int* resultRenderTargetHeight) override;

    void setPixelBuffer(const FrameInfo& info,
                        const unsigned char* buf,
                        size_t len) override {
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)
        mBuffer = buf;
        mBufferLen = len;
    }

private:
    // pinter to the pixel buffer, don't free within this class
    const unsigned char* mBuffer;
    size_t mBufferLen;

private:
    void paintEvent(QPaintEvent*) override;
};

}  // namespace videoplayer
}  // namespace android
