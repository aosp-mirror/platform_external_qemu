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
#include "android/skin/SwipeGesture.h"

#include <gtest/gtest.h>
#include <memory>
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/skin/SwipeGesture.h"

namespace android {
namespace emulation {
namespace control {

using android::base::TestLooper;

class Point {
public:
    Point() : mX(0), mY(0) {}
    Point(int x, int y) : mX(x), mY(y) {}
    int x() const { return mX; }
    int y() const { return mY; }

private:
    int mX;
    int mY;
};

using SwipeTest = SwipeGesture<Point>;

struct TestMouseEvent {
    int x;
    int y;
    int z;
    int buttons;
    int displayId;
};

std::vector<TestMouseEvent> mouseEvents;

QAndroidUserEventAgent userAgent = {
        .sendMouseEvent = [](int x, int y, int z, int buttons, int displayId) {
            mouseEvents.push_back({x, y, z, buttons, displayId});
        }};

TEST(SwipeGestureTest, ImmediateMouseDown) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);

    EXPECT_EQ(mouseEvents.size(), 1);
    EXPECT_EQ(mouseEvents[0].x, 10);
    EXPECT_EQ(mouseEvents[0].y, 10);
    EXPECT_EQ(mouseEvents[0].buttons, 1);
    EXPECT_EQ(mouseEvents[0].displayId, 0);
}

TEST(SwipeGestureTest, BasicSwipeY) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);

    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }
    EXPECT_EQ(mouseEvents.size(), 22);
    // We should have arrived at the end.
    EXPECT_EQ(mouseEvents.back().y, 110);
    EXPECT_EQ(mouseEvents.back().x, 10);
}

TEST(SwipeGestureTest, BasicSwipeX) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{100, 0};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);

    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }
    EXPECT_EQ(mouseEvents.size(), 22);
    // We should have arrived at the end.
    EXPECT_EQ(mouseEvents.back().x, 110);
    EXPECT_EQ(mouseEvents.back().y, 10);
}

TEST(SwipeGestureTest, BasicSwipe) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{100, 100};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);

    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }
    EXPECT_EQ(mouseEvents.size(), 22);
    // We should have arrived at the end.
    EXPECT_EQ(mouseEvents.back().x, 110);
    EXPECT_EQ(mouseEvents.back().y, 110);
}


TEST(SwipeGestureTest, VerticalSwipeNoX) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);
    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    // We don't modify x axis
    for (int i = 1; i < mouseEvents.size(); i++) {
        EXPECT_EQ(mouseEvents[i].x, mouseEvents[i - 1].x);
    }
}

TEST(SwipeGestureTest, HorizontalSwipeNoY) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{100, 0};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);
    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    // We don't modify y axis
    for (int i = 1; i < mouseEvents.size(); i++) {
        EXPECT_EQ(mouseEvents[i].y, mouseEvents[i - 1].y);
    }
}

TEST(SwipeGestureTest, BasicSwipeEndsInMouseUp) {
    mouseEvents.clear();
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};

    SwipeGesture<Point> gesture(&looper, &userAgent, displayId, touchDownPoint,
                                delta);

    gesture.wheelScrollTimeout();
    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }
    EXPECT_EQ(mouseEvents.back().buttons, 0);
}

TEST(SwipeGestureTest, MoveMoreY) {
    mouseEvents.clear();

    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};

    SwipeTest gesture(&looper, &userAgent, displayId, touchDownPoint, delta);

    gesture.wheelScrollTimeout();
    gesture.wheelScrollTimeout();
    EXPECT_EQ(mouseEvents.size(), 3);

    // Move the gesture by 50 units
    gesture.moveMore({0, 50});
    gesture.wheelScrollTimeout();

    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    // We moved 50 more..so we should have arrived at 160
    EXPECT_EQ(mouseEvents.back().y, 160);
}

TEST(SwipeGestureTest, MoveMoreX) {
    mouseEvents.clear();

    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{100, 0};

    SwipeTest gesture(&looper, &userAgent, displayId, touchDownPoint, delta);

    gesture.wheelScrollTimeout();
    gesture.wheelScrollTimeout();
    EXPECT_EQ(mouseEvents.size(), 3);

    // Move the gesture by 50 units
    gesture.moveMore({50, 0});
    gesture.wheelScrollTimeout();

    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    // We moved 50 more..so we should have arrived at 160
    EXPECT_EQ(mouseEvents.back().x, 160);
}

TEST(SwipeGestureTest, RepositionY) {
    mouseEvents.clear();

    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};  // We should arrive at 110

    SwipeTest gesture(&looper, &userAgent, displayId, touchDownPoint, delta);

    // Reposition the gesture to a new touch down point (20, 20) with a delta of
    // 20
    auto newTouchDownPoint = Point(20, 20);

    // Starts the motion from the new touch down point with restoring the
    // previous remaining delta + adding new delta.
    gesture.reposition(newTouchDownPoint, {0, 20});

    gesture.wheelScrollTimeout();
    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    EXPECT_EQ(mouseEvents.back().y, 140);
}

TEST(SwipeGestureTest, RepositionX) {
    mouseEvents.clear();

    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{100, 0};  // We should arrive at 110

    SwipeTest gesture(&looper, &userAgent, displayId, touchDownPoint, delta);

    // Reposition the gesture to a new touch down point (20, 20) with a delta of
    // 20
    auto newTouchDownPoint = Point(20, 20);

    // Starts the motion from the new touch down point with restoring the
    // previous remaining delta + adding new delta.
    gesture.reposition(newTouchDownPoint, {20, 0});

    gesture.wheelScrollTimeout();
    while (!gesture.atEnd()) {
        gesture.wheelScrollTimeout();
    }

    EXPECT_EQ(mouseEvents.back().x, 140);
}

TEST(SwipeGestureTest, LooperMovesTheWheel) {
    mouseEvents.clear();

    android::base::TestSystem testSystem(
            "progdir", base::System::kProgramBitness, "appdir");
    TestLooper looper;

    int displayId = 0;
    auto touchDownPoint = Point(10, 10);
    Point delta{0, 100};  // We should arrive at 110

    SwipeTest gesture(&looper, &userAgent, displayId, touchDownPoint, delta);

    while (!gesture.atEnd()) {
        auto now = testSystem.getHighResTimeUs();
        now += 1000;
        looper.runOneIterationWithDeadlineMs(now / 1000);
        testSystem.setUnixTimeUs(now);
    }

    EXPECT_EQ(mouseEvents.back().y, 110);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
