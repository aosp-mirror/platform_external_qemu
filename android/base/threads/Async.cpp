#include "android/base/threads/Async.h"

#include "android/base/threads/FunctorThread.h"

#include <memory>

namespace android {
namespace base {

namespace {

class SelfDeletingThread final : public FunctorThread {
public:
    using FunctorThread::FunctorThread;

    virtual void onExit() override {
        delete this;
    }
};

}

bool Async(const ThreadFunctor& func, ThreadFlags flags) {
    auto thread =
            std::unique_ptr<SelfDeletingThread>(
                new SelfDeletingThread(func, flags));
    if (thread->start()) {
        thread.release();
        return true;
    }

    return false;
}

}  // namespace base
}  // namespace android
