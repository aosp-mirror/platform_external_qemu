// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/mouse-event-handler.h"

#include "android/skin/qt/emulator-qt-window.h"

#if QT_VERSION >= 0x060000
void MouseEventHandler::handleMouseEvent(SkinEventType type,
                                        SkinMouseButtonType button,
                                        const QPointF& posF,
                                        const QPointF& gPosF,
                                        bool skipSync,
                                        uint32_t displayId) {
    QPoint pos((int) posF.x(), (int) posF.y());
    QPoint gPos((int) gPosF.x(), (int) gPosF.y());
#else
void MouseEventHandler::handleMouseEvent(SkinEventType type,
                                        SkinMouseButtonType button,
                                        const QPoint& pos,
                                        const QPoint& gPos,
                                        bool skipSync,
                                        uint32_t displayId) {
#endif  // QT_VERSION

    SkinEvent* skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = button;
    skin_event->u.mouse.skip_sync = skipSync;
    skin_event->u.mouse.x = pos.x();
    skin_event->u.mouse.y = pos.y();
    skin_event->u.mouse.x_global = gPos.x();
    skin_event->u.mouse.y_global = gPos.y();

    skin_event->u.mouse.xrel = pos.x() - mPrevMousePosition.x();
    skin_event->u.mouse.yrel = pos.y() - mPrevMousePosition.y();
    skin_event->u.mouse.display_id = displayId;
    mPrevMousePosition = pos;
    auto win = EmulatorQtWindow::getInstance();
    win->queueSkinEvent(skin_event);
}

// State machine that translates the mouse event types based on button states
// If during a touching state the button is pressed this generates unwanted
// Press and Release events which are translated to Move events
SkinEventType MouseEventHandler::translateMouseEventType(SkinEventType type,
                                                    Qt::MouseButton button,
                                                    Qt::MouseButtons buttons) {
    SkinEventType newType = type;

    switch (mMouseTouchState) {
    case TouchState::NOT_TOUCHING:
        // Only the first Press event can have the same pressed buttons
        // button:  only the button that caused the event
        // buttons: all the pressed buttons
        if ((type == kEventMouseButtonDown) && (button == buttons)) {
            mMouseTouchState = TouchState::TOUCHING;
            newType = kEventMouseButtonDown;
        } else {
            newType = kEventMouseButtonUp;
        }
        break;
    case TouchState::TOUCHING:
        // Only the last Release event can have no pressed buttons
        if ((type == kEventMouseButtonUp) && (buttons == Qt::NoButton)) {
            mMouseTouchState = TouchState::NOT_TOUCHING;
            newType = kEventMouseButtonUp;
        } else {
            newType = kEventMouseMotion;
        }
        break;
    default:
        mMouseTouchState = TouchState::NOT_TOUCHING;
        newType = kEventMouseButtonUp;
        break;
    }

    return newType;
}

SkinMouseButtonType MouseEventHandler::getSkinMouseButton(
                                    const QMouseEvent* event) const {
    switch (event->button()) {
    case Qt::LeftButton:
        return kMouseButtonLeft;
    case Qt::RightButton:
        return kMouseButtonRight;
    case Qt::MiddleButton:
        return kMouseButtonMiddle;
    default:
        return kMouseButtonLeft;
    }
}

SkinEvent* MouseEventHandler::createSkinEvent(SkinEventType type) {
    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = type;
    return skin_event;
}
