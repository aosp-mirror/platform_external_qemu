#include "emulator/net/AsyncSocketAdapter.h"

#if DEBUG >= 2
#define DD(fmt, ...)                                                     \
    do {                                                                 \
        printf("AsyncSocketAdapter: %s:%d| " fmt "\n", __func__, __LINE__, \
               ##__VA_ARGS__);                                           \
    } while (0)
#else
#define DD(fmt, ...) (void)0
#endif


namespace emulator {
namespace net {

void AsyncSocketAdapter::addSocketEventListener(
        AsyncSocketEventListener* listener) {
    std::unique_lock<std::mutex> lk(mListenerMutex);
    mCv.wait(lk, [this] { return mRead == 0; });
    mListeners.insert(listener);
    DD("%lu", mListeners.size());
    lk.unlock();
    mCv.notify_one();
}

void AsyncSocketAdapter::removeSocketEventListener(
        AsyncSocketEventListener* listener) {
    std::unique_lock<std::mutex> lk(mListenerMutex);
    mCv.wait(lk, [this] { return mRead == 0; });
    mListeners.erase(listener);
    DD("%lu", mListeners.size());
    lk.unlock();
    mCv.notify_one();
}

void AsyncSocketAdapter::removeAllEventListeners() {
    std::unique_lock<std::mutex> lk(mListenerMutex);
    mListeners.clear();
    DD("%lu", mListeners.size());
}

void AsyncSocketAdapter::notify(Notification type, int err) {
    // No read events for the disconnected.
    if (type == Notification::Read && !connected())
        return;

    {
        std::lock_guard<std::mutex> lk(mListenerMutex);
        mRead++;
    }
    mCv.notify_one();
    for (auto listener : mListeners) {
        switch (type) {
            case Notification::Read:
                DD("Read: %p", listener);
                listener->onRead(this);
                break;
            case Notification::Close:
                DD("Close: %p", listener);
                listener->onClose(this, err);
                break;
            case Notification::Connected:
                DD("Connected: %p", listener);
                listener->onConnected(this);
                break;
        }
    }
    {
        std::lock_guard<std::mutex> lk(mListenerMutex);
        mRead--;
    }
    mCv.notify_one();
}
}  // namespace net
}  // namespace emulator
