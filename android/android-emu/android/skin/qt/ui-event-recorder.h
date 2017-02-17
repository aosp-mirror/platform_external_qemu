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

#include "android/base/TypeTraits.h"
#include "android/base/memory/LazyInstance.h"
#include "android/skin/qt/event-subscriber.h"

#include <QEvent>
#include <QEnterEvent>
#include <QCloseEvent>
#include <QHideEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QResizeEvent>

#include <cassert>
#include <memory>
#include <unordered_map>

using android::base::enable_if_convertible;

struct EventRecord {
    std::unique_ptr<QEvent> event;
    std::string target_name;
};

struct EventSetHolder {
    EventSetHolder();
    EventCapturer::EventTypeSet events;
};

extern android::base::LazyInstance<EventSetHolder> sEventRecorderEventsHolder;

// This class intercepts various UI events via EventCapturer
// and stores their serialized representations in a container.
template <template<class T, class A = std::allocator<T>> class EventContainer>
class UIEventRecorder : public EventSubscriber {
public:
    using ContainerType = EventContainer<EventRecord>;

    explicit UIEventRecorder(EventCapturer* ecap) : EventSubscriber(ecap) {}

    // Lets the client specialize their own container instance.
    template <typename U>
    explicit UIEventRecorder(
            EventCapturer* ecap,
            U&& container,
            enable_if_convertible<U, ContainerType> = nullptr) :
        EventSubscriber(ecap),
        mContainer(std::forward<U>(container)) {}

    // Get a const reference to the underelying container with serialized
    // events.
    const ContainerType& container() const {
        return mContainer;
    }

private:
    bool objectPredicate(const QObject*) const override { return true; }

    const EventCapturer::EventTypeSet& eventTypes() const override {
        return sEventRecorderEventsHolder->events;
    }

    void processEvent(const QObject* target, const QEvent* event) override {
        EventRecord record {
            cloneEvent(event),
            target->objectName().toStdString()
        };
        mContainer.push_back(std::move(record));
    }

    std::unique_ptr<QEvent> cloneEvent(const QEvent* e) {
        switch(e->type()) {
        case QEvent::Close:
            return cloneEventHelper<QCloseEvent>(e);
        case QEvent::Enter:
            return cloneEventHelper<QEnterEvent>(e);
        case QEvent::Leave:
            return cloneEventHelper<QEvent>(e);
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            return cloneEventHelper<QMouseEvent>(e);
        case QEvent::FocusIn:
        case QEvent::FocusOut:
            return cloneEventHelper<QFocusEvent>(e);
        case QEvent::Hide:
            return cloneEventHelper<QHideEvent>(e);
        case QEvent::Resize:
            return cloneEventHelper<QResizeEvent>(e);
        default:
            // This is a safety measure. If we got here, it means
            // that this method was not updated to match the supoported
            // event types.
            assert(false);
        }
        return nullptr;
    }

    template <class T>
    std::unique_ptr<QEvent> cloneEventHelper(
            const QEvent* e, enable_if_convertible<T*, QEvent*> = nullptr) {
        return std::unique_ptr<QEvent>(new T(*static_cast<const T*>(e)));
    }

private:
    ContainerType mContainer;
};
