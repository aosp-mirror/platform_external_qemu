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

#include "android/base/TypeUtils.h"
#include "android/skin/qt/event-subscriber.h"

#include <QEvent>
#include <QEnterEvent>
#include <QCloseEvent>
#include <QHideEvent>
#include <QFocusEvent>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QString>
#include <QTime>

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>

using android::base::enable_if_convertible;

struct EventRecord {
    std::unique_ptr<QEvent> event;
    std::string target_name;
    int dt;

    inline std::string to_string() const {
        static int event_enum_index =
                QEvent::staticMetaObject.indexOfEnumerator("Type");
        std::ostringstream str;
        str << dt << " : " << target_name << " : "
            << QEvent::staticMetaObject.enumerator(event_enum_index)
                        .valueToKey(event->type());

        return str.str();
    }
};

/*****************************************************************************/

// This class intercepts various UI events via EventCapturer
// and stores their serialized representations in a container.
template <template<class T, class A = std::allocator<T>> class EventContainer>
class UIEventRecorder : public EventSubscriber {
public:
    using ContainerType = EventContainer<EventRecord>;

    explicit UIEventRecorder(EventCapturer* ecap)
        : EventSubscriber(ecap), mTimeStamper() {
        mTimeStamper.start();
    }

    // Lets the client specialize their own container instance.
    template <typename U>
    explicit UIEventRecorder(EventCapturer* ecap,
                             U&& container,
                             enable_if_convertible<U, ContainerType> = nullptr)
        : EventSubscriber(ecap),
          mContainer(std::forward<U>(container)),
          mTimeStamper() {
        mTimeStamper.start();
    }

    void print() {
        for (int i = 0; i < mContainer.size(); ++i) {
            fprintf(stderr, "*** %d : %s  ***\n", mContainer[i].dt,
                    mContainer[i].to_string().c_str());
        }
    }

    // Get a const reference to the underlying container with serialized
    // events.
    const ContainerType& container() const {
        return mContainer;
    }

    // Get a reference to the underlying container for manipulation
    ContainerType& wContainer() { return mContainer; }

private:
    bool objectPredicate(const QObject*) const override { return true; }

    const EventCapturer::EventTypeSet& eventTypes() const override {
        return mEventTypes;
    }

    std::unique_ptr<QEvent> cloneEvent(const QEvent* e) {
        switch(e->type()) {
        case QEvent::Close:
            return cloneEventHelper<QCloseEvent>(e);
        case QEvent::Enter:
            return cloneEventHelper<QEnterEvent>(e);
        case QEvent::Leave:
            return cloneEventHelper<QEvent>(e);
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
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

    void processEvent(const QObject* target, const QEvent* event) override {
        EventRecord record{cloneEvent(event),
                           target->objectName().toStdString(),
                           mTimeStamper.elapsed()};
        mTimeStamper.restart();
        mContainer.push_back(std::move(record));
    }

    template <class T>
    std::unique_ptr<QEvent> cloneEventHelper(const QEvent* e, enable_if_convertible<T*, QEvent*> = nullptr) {
        return std::unique_ptr<QEvent>(new T(*static_cast<const T*>(e)));
    }

private:
    ContainerType mContainer;
    static EventCapturer::EventTypeSet mEventTypes;
    QTime mTimeStamper;
};

template <template <class T, class A = std::allocator<T>> class EventContainer>
EventCapturer::EventTypeSet UIEventRecorder<EventContainer>::mEventTypes = {
        QEvent::Close,
        QEvent::Enter,
        QEvent::Leave,
        QEvent::FocusIn,
        QEvent::FocusOut,
        QEvent::Hide,
        QEvent::MouseButtonDblClick,
        QEvent::MouseButtonPress,
        QEvent::MouseButtonRelease,
        QEvent::MouseMove,
        QEvent::Resize};
