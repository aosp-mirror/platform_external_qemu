
#include <unordered_set>      // for unordered_set

#include "android/console.h"  // for AndroidConsoleAgents

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace control {
class TouchEvent;

using base::Looper;

// Class that sends touch events on the current looper.
class TouchEventSender {
public:
    TouchEventSender(const AndroidConsoleAgents* const consoleAgents);
    ~TouchEventSender();
    bool send(const TouchEvent* request);
    bool sendOnThisThread(const TouchEvent* request);

private:
    void doSend(const TouchEvent touch);

    const AndroidConsoleAgents* const mAgents;
    std::unordered_set<int> mUsedSlots;
    Looper* mLooper;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
