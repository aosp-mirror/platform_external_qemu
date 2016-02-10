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

#include "android/skin/qt/event-subscriber.h"
#include <type_traits>
#include <unordered_map>

// This class intercepts various UI events via EventCapturer
// and stores their serialized representations in a container.
template <template<class T, class A = std::allocator<T>> class EventContainer>
class UIEventRecorder : public EventSubscriber {
public:
    explicit UIEventRecorder(EventCapturer* ecap) : EventSubscriber(ecap) {}

    // Lets the client specialize their own container instance.
    template <typename U>
    explicit UIEventRecorder(
            U&& container,
            typename std::enable_if<
                std::is_convertible<U, EventContainer<QString>>::value,
                int>::type dummy = 0) :
        mContainer(std::forward(container)) {}

    // Get a const reference to the underelying container with serialized
    // events.
    const EventContainer<QString>& container() const {
        return mContainer;
    }

private:
    bool objectPredicate(const QObject*) const override { return true; }

    const EventCapturer::EventTypeSet& eventTypes() const override {
        return mEventTypes;
    }
 
    void processEvent(const QObject* target, const QEvent* event) override {
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

private:
    EventContainer<QString> mContainer;
    static EventCapturer::EventTypeSet mEventTypes;
};


template <template<class T, class A = std::allocator<T>> class EventContainer>
EventCapturer::EventTypeSet UIEventRecorder<EventContainer>::mEventTypes = {
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
};

