
#include <unordered_set>      // for unordered_set
#include <unordered_map>      // for unordered_map
#include "android/console.h"  // for AndroidConsoleAgents
#include <chrono>

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
    int pickNextSlot();

    const AndroidConsoleAgents* const mAgents;

    // Set of active slots..
    std::unordered_set<int> mUsedSlots;

     // Maps external touch id --> linux slot
    std::unordered_map<int, int> mIdMap;

    // Last time external touch id was used, we use this to expunge
    // dangling events
    std::unordered_map<int, std::chrono::seconds> mIdLastUsedEpoch;

    Looper* mLooper;

    // We expire touch events after 120 seconds. This means that if
    // a given id has not received any updates in 120 seconds it will
    // be closed out upon receipt of the next event.
    const std::chrono::seconds kTOUCH_EXPIRE_AFTER_120S{120};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
