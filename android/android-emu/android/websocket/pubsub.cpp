#include "android/websocket/pubsub.h"
#include "android/base/Log.h"

using namespace android::base;

void Pubsub::connect(Receiver* r) {
    mWorkerQueue.submit([this, r]() { this->mConnected.insert(r); });
}

void Pubsub::subscribe(Receiver* r, Topic t) {
    mWorkerQueue.submit([this, r, t]() {
        // Note that if t not in mTopics a default object will be created.
        LOG(VERBOSE) << "subscribe: " << r << ", topic: " << t;
        mTopics[t].insert(r);
        mSubscribed[r].insert(t);
    });
}

void Pubsub::unsubscribe(Receiver* r, Topic t) {
    LOG(VERBOSE) << "unsubscribe: " << r << ", topic: " << t;
    mWorkerQueue.submit([this, r, t]() {
        if (mTopics.find(t) != mTopics.end()) {
            mTopics[t].erase(r);
            if (mTopics[t].empty()) {
                mTopics.erase(t);
            }
        }

        if (mSubscribed.find(r) != mSubscribed.end()) {
            mSubscribed[r].erase(t);
            if (mSubscribed[r].empty()) {
                mSubscribed.erase(r);
            }
        }
    });
}

void Pubsub::publish(Receiver* from, Topic t, std::string msg) {
    mWorkerQueue.submit([this, from, t, msg]() {
        LOG(VERBOSE) << "publish: " << t << ", msg: " << msg;
        if (mTopics.find(t) == mTopics.end()) {
            return;
        }
        for (auto rec : mTopics[t]) {
            LOG(VERBOSE) << "Sending to " << rec;
            rec->send(from, t, msg);
        }
    });
}

void Pubsub::disconnect(Receiver* r) {
    mWorkerQueue.submit([this, r]() {
        LOG(VERBOSE) << "disconnect: " << r;
        assert(mConnected.find(r) != mConnected.end());
        for (auto topic : mSubscribed[r]) {
            mTopics[topic].erase(r);
            if (mTopics[topic].empty()) {
              mTopics.erase(topic);
            }
        }
        mSubscribed.erase(r);
        mConnected.erase(r);
    });
}
