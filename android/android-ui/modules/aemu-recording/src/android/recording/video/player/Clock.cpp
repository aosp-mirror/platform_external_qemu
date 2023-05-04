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

// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "android/recording/video/player/Clock.h"

extern "C" {
#include "libavutil/time.h"
}

#include <math.h>

namespace android {
namespace videoplayer {

// Clock implementations
void Clock::init(int* queue_serial) {
    mSpeed = 1.0;
    mPaused = false;
    mQueueSerialPtr = queue_serial;
    set(NAN, -1);
}

double Clock::getTime() {
    if (*mQueueSerialPtr != mSerial) {
        return NAN;
    }
    if (mPaused) {
        return mPts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return mPtsDrift + time - (time - mLastUpdated) * (1.0 - mSpeed);
    }
}

void Clock::set(double pts, int serial) {
    double time = av_gettime_relative() / 1000000.0;
    setAt(pts, serial, time);
}

void Clock::setAt(double pts, int serial, double time) {
    mPts = pts;
    mLastUpdated = time;
    mPtsDrift = mPts - time;
    mSerial = serial;
}

void Clock::setSpeed(double speed) {
    double clock = this->getTime();
    this->set(clock, mSerial);
    mSpeed = speed;
}

void Clock::syncToSlave(Clock* slave) {
    double clock = this->getTime();
    double slave_clock = slave->getTime();
    if (!isnan(slave_clock) &&
        (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        this->set(slave_clock, slave->mSerial);
    }
}

}  // namespace videoplayer
}  // namespace android
