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

#pragma once

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.04
// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1
// If a frame duration is longer than this, it will not be duplicated to
// compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

// external clock speed adjustment constants for realtime sources based on
// buffer fullness
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

namespace android {
namespace videoplayer {

class Clock {
public:
    Clock() = default;
    ~Clock() = default;

    void init(int* queue_serial);
    double getTime();
    void set(double pts, int serial);
    void setAt(double pts, int serial, double time);
    void setSpeed(double speed);
    void syncToSlave(Clock* slave);

    int getSerial() const { return mSerial; }

    int* getSerialPtr() { return &mSerial; }

    int getSpeed() const { return mSpeed; }

private:
    // clock base
    double mPts = 0.0;

    // clock base minus time at which we updated the clock
    double mPtsDrift = 0.0;
    double mLastUpdated = 0.0;
    double mSpeed = 0.0;

    // clock is based on a packet with this serial
    int mSerial = 0.0;

    bool mPaused = false;

    // pointer to the current packet queue serial, used for obsolete clock
    // detection
    int* mQueueSerialPtr = nullptr;
};

}  // namespace videoplayer
}  // namespace android
