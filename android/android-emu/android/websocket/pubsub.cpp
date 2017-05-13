#include "android/websocket/pubsub.h"
#include "android/base/log.h"

using namespace android::base;

void Pubsub::connect(Receiver* r) {
    AutoLock lock(mLock);
    LOG(INFO) << "connect: " << r;
    assert(mConnected.find(r) == mConnected.end());
    mConnected.insert(r);
}

void Pubsub::subscribe(Receiver* r, Topic t) {
    AutoLock lock(mLock);
    LOG(INFO) << "subscribe: " << r << ", topic: " << t;
    // Note that if t not in mTopics a default object will be created.
    mTopics[t].insert(r);
    mSubscribed[r].insert(t);
}

void Pubsub::unsubscribe(Receiver* r, Topic t) {
    AutoLock lock(mLock);
    LOG(INFO) << "unsubscribe: " << r << ", topic: " << t;
    if (mTopics.find(t) != mTopics.end()) {
        auto set = mTopics[t];
        set.erase(r);
        if (set.empty()) {
            mTopics.erase(t);
        }
    }

    if (mSubscribed.find(r) != mSubscribed.end()) {
        auto set = mSubscribed[r];
        set.erase(t);
        if (set.empty()) {
            mSubscribed.erase(r);
        }
    }
}

void Pubsub::publish(Topic t, std::string msg) {
    std::set<Receiver*> receivers;
    {
        AutoLock lock(mLock);
        LOG(INFO) << "publish: " << t << ", msg: " << msg;
        if (mTopics.find(t) == mTopics.end()) {
            return;
        }
        auto topics = mTopics[t];
        // Let's make a copy so we can release the lock
        receivers = topics;
    }

    for (auto rec : receivers) {
        LOG(INFO) << " to " << rec;
        rec->send(msg);
        LOG(INFO) << " done" << rec;
    }
    LOG(INFO) << "done: " << t << ", msg: " << msg;
}

void Pubsub::disconnect(Receiver* r) {
    AutoLock lock(mLock);
    LOG(INFO) << "disconnect: " << r;
    assert(mConnected.find(r) != mConnected.end());
    // TODO, if we are not mSubscribed we are constructing and removing
    // an object..
    for (auto topic : mSubscribed[r]) {
        // TODO we are leaking mTopics here.
        // we should remove a topic if the subscriber set is empty
        auto receivers = mTopics[topic];
        receivers.erase(r);
        if (receivers.empty()) {
            LOG(WARNING) << " leaking empty topic: " << topic;
        }
    }
    mSubscribed.erase(r);
    mConnected.erase(r);
}
