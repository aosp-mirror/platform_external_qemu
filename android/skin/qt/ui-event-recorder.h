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
#include <QEvent>
#include <QCloseEvent>
#include <QHideEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <memory>
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
                std::is_convertible<U, EventContainer<std::unique_ptr<QEvent>>>::value,
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
        mContainer.push_back(cloneEvent(e));
    }

    std::unique_ptr<QEvent> cloneEvent(const QEvent* e) {
        switch(e->type()) {
        case QEvent::Close:
            return std::unique_ptr<QEvent>(new QCloseEvent(*dynamic_cast<QCloseEvent>(e));
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            return std::unique_ptr<QEvent>(new QMouseEvent(*dynamic_cast<QMouseEvent>(e));
        case QEvent::FocusIn:
        case QEvent::FocusOut:
            return std::unique_ptr<QEvent>(new QFocusEvent(*dynamic_cast<QFocusEvent>(e));
        case QEvent::Hide:
            return std::unique_ptr<QEvent>(new QHideEvent(*dynamic_cast<QHideEvent>(e));
        case QEvent::Resize:
            return std::unique_ptr<QEvent>(new QResizeEvent(*dynamic_cast<QResizeEvent>(e));
        default:
            // This is a safety measure. If we got here, it means
            // that this method was not updated to match the supoported
            // event types.
            assert(false);
        }
    }

private:
    EventContainer<std::unique_ptr<QEvent>> mContainer;
    static EventCapturer::EventTypeSet mEventTypes;
};


template <template<class T, class A = std::allocator<T>> class EventContainer>
EventCapturer::EventTypeSet UIEventRecorder<EventContainer>::mEventTypes = {
    QEvent::Close,
    QEvent::Enter,
    QEvent::Leave,
    QEvent::FocusIn,
    QEvent::FocusOut,
    QEvent::Hide,
    QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease,
    QEvent::Resize
};

