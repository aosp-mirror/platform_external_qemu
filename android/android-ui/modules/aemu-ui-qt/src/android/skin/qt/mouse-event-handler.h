// Copyright 2022 The Android Open Source Project
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

#include <QMouseEvent>

#include "android/skin/event.h"
#include "android/skin/qt/emulator-overlay.h"

class MouseEventHandler {
public:
    MouseEventHandler() :
        mPrevMousePosition(0, 0),
        mMouseTouchState(TouchState::NOT_TOUCHING) {}
#if QT_VERSION >= 0x060000
    void handleMouseEvent(SkinEventType type,
                          SkinMouseButtonType button,
                          const QPointF& pos,
                          const QPointF& gPos,
                          bool skipSync,
                          uint32_t displayId);
#else
    void handleMouseEvent(SkinEventType type,
                          SkinMouseButtonType button,
                          const QPoint& pos,
                          const QPoint& gPos,
                          bool skipSync,
                          uint32_t displayId);
#endif  // QT_VERSION


    SkinEventType translateMouseEventType(SkinEventType type,
                                          Qt::MouseButton button,
                                          Qt::MouseButtons buttons);
    SkinMouseButtonType getSkinMouseButton(const QMouseEvent* event) const;
private:
    SkinEvent* createSkinEvent(SkinEventType type);
    TouchState mMouseTouchState;
    QPoint mPrevMousePosition;
};
