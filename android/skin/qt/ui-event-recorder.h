// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/event-capture.h"
#include <unordered_map>

// This class intercepts various UI events via EventCapture
// and stores their serialized representations in a container.
template <template<class T, class A = std::allocator<T>> class EventContainer>
class UIEventRecorder {
public:
    UIEventRecorder(){}

    // Lets the client specialize their own container instance.
    explicit UIEventRecorder(EventContainer<QString>&& container) :
        mContainer(std::forward(container)) {}

    // Start monitoring UI events from the given widget's hierarchy.
    void startRecording(EventCapture* ecap, QWidget* widget) {
        mEventCaptureTokens[widget] = ecap->subscribeToEvents(
                widget, // subscribe to events from |widget|'s hierarchy.
                [](const QObject*){ return true; }, // no filtering for children
                { // Types of events we're interested in.
                    QEvent::Close,
                    QEvent::ContentsRectChange,
                    QEvent::DragEnter,
                    QEvent::DragLeave,
                    QEvent::Drop,
                    QEvent::Enter,
                    QEvent::FocusIn,
                    QEvent::FocusOut,
                    QEvent::Hide,
                    QEvent::KeyPress,
                    QEvent::KeyRelease,
                    QEvent::MouseButtonPress,
                    QEvent::MouseButtonRelease,
                    QEvent::Move,
                    QEvent::Resize
                },
                [this](const QObject* target, const QEvent* event) { // Handler.
                    this->processEvent(target, event);
                });
    }

    // Stop monitoring events for the given widget's hierarchy.
    void stopRecording(QWidget* widget) {
        auto it = mEventCaptureTokens.find(widget);
        if (it != mEventCaptureTokens.end()) {
            *it = EventCapture::SubscriberToken();
        }
    }

    // Get a const reference to the underelying container with serialized
    // events.
    const EventContainer<QString>& container() const {
        return mContainer;
    }

private:
    void processEvent(const QObject* target, const QEvent* event) {
        // TODO: add true serialization, recording time information and
        // full event information.
        int ev_type_enum_index =
            QEvent::staticMetaObject.indexOfEnumerator("Type");
        QString event_type_name =
            QEvent::staticMetaObject
                .enumerator(ev_type_enum_index)
                .valueToKey(event->type());
        mContainer.push_back("[" + target->objectName() + "] " +
                             event_type_name);
    }

    EventContainer<QString> mContainer;
    std::unordered_map<QWidget*, EventCapture::SubscriberToken> mEventCaptureTokens;
};

