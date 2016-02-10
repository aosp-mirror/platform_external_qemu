// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/event-capturer.h"
#include <queue>

EventCapturer::~EventCapturer() {
    // Invalidate all the tokens dispensed by this instance.
    // Failing to do that would cause the tokens to try and 
    // call into this instance upon destruction, resulting
    // in undefined behavior.
    for (const auto& subscriber : mSubscribers) {
        if (subscriber.token) {
            subscriber.token->invalidate();
        }
    }
}

EventCapturer::SubscriberToken EventCapturer::subscribeToEvents(
    QObject* root,
    const EventCapturer::ObjectPredicate& child_predicate,
    const EventCapturer::EventTypeSet& event_types,
    const EventCapturer::Callback& callback) {
    // This will contain the set of objects whose events the caller
    // should be subscribed to.
    std::unordered_set<QObject*> objects;

    // Traverse the object hierarchy starting at root. For each object,
    // check the value of |child_predicate|. If it is true, include it
    // into the set of monitored objects.
    std::queue<QObject*> q;
    q.push(root);
    while (!q.empty()) {
        QObject* obj = q.front();
        q.pop();
        for (QObject* child : obj->children()) {
            // Note that we do NOT check |child_predicate| for
            // the object's children at this point.
            // The idea is, even if the object itself might be
            // excluded, its children should still be considered
            // for inclusion.
            q.push(child);
        }
        if (child_predicate(obj)) {
            objects.insert(obj);
            obj->installEventFilter(this);
        }
    }

    // Add an entry with information needed for subscriber book-keeping.
    mSubscribers.emplace_back();
    auto subscriber_it = std::next(mSubscribers.end(), -1);
    subscriber_it->event_types = event_types;
    subscriber_it->objects = std::move(objects);
    subscriber_it->callback = callback;

    // Create a new token that calls |unsub| upon destruction.
    SubscriberToken token(new internal::SubscriberTokenImpl(
                [this, subscriber_it]() { this->unsub(subscriber_it); }));
    
    // Ensure that this instance can invalidate the token.
    subscriber_it->token = token.get();

    // Give the token to the subscriber.
    return token;
}

void EventCapturer::unsub(std::vector<SubscriberInfo>::iterator it) {
    // Remove associated subscriber info.
    // Order of subscribers isn't relevant so we can do this
    // "fast remove" trick.
    std::swap(*it, mSubscribers.back());
    mSubscribers.pop_back();
}

bool EventCapturer::eventFilter(QObject* target, QEvent* event) {
    // Indicates whether |target| still has any subscribers
    // interested in its events.
    bool has_subscribers = false;

    for (auto& subscriber : mSubscribers) {
        if (subscriber.objects.count(target)) {
            has_subscribers = true;
            if(subscriber.event_types.count(event->type())) {
                subscriber.callback(target, event);
            }
        }
    }

    if (!has_subscribers) {
        // If there are no subscribers interested in events from
        // this object, it is safe to remove the event filter for
        // it.
        target->removeEventFilter(this);
    }

    return QObject::eventFilter(target, event);
}

