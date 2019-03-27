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

#include "android/recording/video/player/VideoPlayerRenderTarget.h"

#include <QtWidgets/QWidget>

namespace android {
namespace videoplayer {

class VideoPlayerWidget : public QWidget, public VideoPlayerRenderTarget {
    Q_OBJECT

public:
    VideoPlayerWidget(QWidget* parent = nullptr);
    ~VideoPlayerWidget();

    void getRenderTargetSize(float sampleAspectRatio,
                             int video_width,
                             int video_height,
                             int* resultRenderTargetWidth,
                             int* resultRenderTargetHeight) override;

    void setPixelBuffer(const FrameInfo& info,
                        const unsigned char* buf,
                        size_t len) override {
        mBuffer = buf;
        mBufferLen = len;
    }

private:
    // pinter to the pixel buffer, don't free within this class
    const unsigned char* mBuffer;
    size_t mBufferLen;

private:
    void paintEvent(QPaintEvent*);
};

}  // namespace videoplayer
}  // namespace android
