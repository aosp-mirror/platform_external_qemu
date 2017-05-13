#include "android/websocket/pubsub.h"
using namespace android::base;

void Pubsub::connect(Receiver* r) {
   AutoLock lock(mLock);
   assert(mConnected.find(r) == mConnected.end());
   mConnected.insert(r);
}

void Pubsub::subscribe(Receiver* r, Topic t) {
  AutoLock lock(mLock);
  // Note that if t not in mTopics a default object will be created.
  mTopics[t].insert(r);
  mSubscribed[r].insert(t);
}

void Pubsub::unsubscribe(Receiver* r, Topic t) {
  AutoLock lock(mLock);
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
   AutoLock lock(mLock);
   if (mTopics.find(t) == mTopics.end()) {
      return;
   }
   for(auto rec : mTopics[t]) {
      rec->send(msg);
   }
}

void Pubsub::disconnect(Receiver* r) {
   AutoLock lock(mLock);
   assert(mConnected.find(r) != mConnected.end());
   // TODO, if we are not mSubscribed we are constructing and removing 
   // an object..
   for(auto topic : mSubscribed[r]) {
     // TODO we are leaking mTopics here.
     // we should remove a topic if the subscriber set is empty
     auto receivers = mTopics[topic];
     receivers.erase(r);
     if (receivers.empty()) {
       fprintf(stderr, "%s Empty topic: %s\n", __func__, topic.c_str());
     }
   }
   mSubscribed.erase(r);
   mConnected.erase(r);
}

