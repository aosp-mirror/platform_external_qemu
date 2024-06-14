// Copyright (C) 2024 The Android Open Source Project
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
#include "aemu/base/async/RecurrentTask.h"
#include "android/emulation/control/user_event_agent.h"

namespace android {
namespace emulation {
namespace control {

template <typename N>
struct QuadraticMotion {
    int length;
    N a;
    N b;
    N c;

    N get(int x) const { return (a * x * x + b * x + c) / (length * length); }

    // Fits f(x) = ax^2 + bx + c, so that it satisfies the following boundary
    // conditions.
    // f(0) = y0
    // f(length) = y1
    // df/dx(length) = 2x + b = 0
    static QuadraticMotion smoothEnd(int length, N y0, N y1) {
        return {
                length,
                y0 - y1,
                -2 * length * y0 + 2 * length * y1,
                length * length * y0,
        };
    }
};

/**
 * @brief This class can be used to translate wheel events into a series of
 * mouse events.
 *
 * The class uses a quadratic motion to simulate a smooth swipe gesture
 * versus a raw WheelEvent. You can use this to "simulate" wheel events if
 * you system-image does not support the WheelEvent.
 *
 * The class can be used to simulate a swipe gesture in either direction.
 * The direction of the swipe is determined by the sign of the delta
 * parameter passed to the constructor.
 *
 * @tparam P The type of the point used to represent the touch down point.
 */
template <typename P>
class SwipeGesture {
    constexpr static int kWheelCount = 20;
    constexpr static int kNoButton = 0;
    constexpr static int kLeftButton = 1;

public:
    using N = decltype(P().y());
    using Vector2d = P;

    SwipeGesture(base::Looper* looper,
                 const QAndroidUserEventAgent* userAgent,
                 int displayId,
                 P touchDownPoint,
                 Vector2d delta)
        : mUserAgent(userAgent),
          mDisplayId(displayId),
          mTouchDownPoint(touchDownPoint),
          mMotionX(QuadraticMotion<N>::smoothEnd(kWheelCount, 0, delta.x())),
          mMotionY(QuadraticMotion<N>::smoothEnd(kWheelCount, 0, delta.y())),
          mScrollTimer(looper, [this]() { return wheelScrollTimeout(); }, 15) {
        start();
    }

    void start() {
        mouseDown();
        mScrollTimer.start();
    }

    void tick() {
        if (!atEnd()) {
            mTick++;
        }
    }

    // Starts the motion from the current touch point to the previous
    // destination point shifted by delta.
    void moveMore(Vector2d delta) {
        auto newStartPoint = mMotionX.get(mTick);
        mTick = 0;
        mMotionX = QuadraticMotion<N>::smoothEnd(
                kWheelCount, newStartPoint,
                mMotionX.get(mMotionX.length) + delta.x());
        mMotionY = QuadraticMotion<N>::smoothEnd(
                kWheelCount, mMotionY.get(mMotionY.length),
                mMotionY.get(mMotionY.length) + delta.y());
        mouseDown();
    }

    // Starts the motion from the new touch down point with restoring
    // the previous remaining delta + adding new delta.
    void reposition(P touchDownPoint, Vector2d delta) {
        mouseUp();
        auto newDelta = P(
                mMotionX.get(mMotionX.length) - mMotionX.get(mTick) + delta.x(),
                mMotionY.get(mMotionY.length) - mMotionY.get(mTick) +
                        delta.y());
        mTick = 0;
        mTouchDownPoint = touchDownPoint;
        mMotionX = QuadraticMotion<N>::smoothEnd(kWheelCount, 0, newDelta.x());
        mMotionY = QuadraticMotion<N>::smoothEnd(kWheelCount, 0, newDelta.y());
        mouseDown();
    }

    void mouseDown() {
        auto location = point();
        mUserAgent->sendMouseEvent(location.x(), location.y(), 0, kLeftButton,
                                   mDisplayId);
    }

    void mouseUp() {
        auto location = point();
        mUserAgent->sendMouseEvent(location.x(), location.y(), 0, kNoButton,
                                   mDisplayId);
    }

    P point() const {
        auto s = swiped();
        return {mTouchDownPoint.x() + s.x(), mTouchDownPoint.y() + s.y()};
    }

    bool atEnd() const { return mTick == mMotionX.length; }

    P swiped() const { return {mMotionX.get(mTick), mMotionY.get(mTick)}; }

    bool wheelScrollTimeout() {
        tick();
        mouseDown();
        if (atEnd()) {
            mouseUp();
        }
        return !atEnd();
    }

private:
    QuadraticMotion<N> mMotionX;
    QuadraticMotion<N> mMotionY;
    P mTouchDownPoint;
    int mTick{0};
    int mDisplayId;
    base::RecurrentTask mScrollTimer;
    const QAndroidUserEventAgent* mUserAgent;
};
}  // namespace control
}  // namespace emulation
}  // namespace android