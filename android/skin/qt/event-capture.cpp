// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/event-capture.h"
#include <queue>

EventCapture::~EventCapture() {
    for (const auto& subscriber : mSubscribers) {
        if (subscriber.token) {
            subscriber.token->invalidate();
        }
    }
}

EventCapture::SubscriberToken EventCapture::subscribeToEvents(
    QObject* root,
    const EventCapture::ObjectPredicate& child_predicate,
    const EventCapture::EventTypeSet& event_types,
    const EventCapture::Callback& callback) {
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

    mSubscribers.emplace_back();
    auto subscriber_it = std::next(mSubscribers.end(), -1);
    subscriber_it->event_types = event_types;
    subscriber_it->objects = std::move(objects);
    subscriber_it->callback = callback;

    SubscriberToken token(new detail::SubscriberTokenImpl(
                [this, subscriber_it]() { this->unsub(subscriber_it); }));
    subscriber_it->token = token.get();
    return token;
}

void EventCapture::unsub(std::vector<SubscriberInfo>::iterator it) {
    std::swap(*it, mSubscribers.back());
    mSubscribers.pop_back();
}

bool EventCapture::eventFilter(QObject* target, QEvent* event) {
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
        target->removeEventFilter(this);
    }
    return QObject::eventFilter(target, event);
}

