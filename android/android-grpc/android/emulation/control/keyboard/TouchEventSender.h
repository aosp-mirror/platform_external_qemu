
#include <unordered_map>
#include <unordered_set>

#include "android/base/synchronization/Lock.h"
#include "android/console.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

using base::Looper;

// Class that sends touch events on the current looper.
class TouchEventSender {
public:
    TouchEventSender(const AndroidConsoleAgents* const consoleAgents);
    ~TouchEventSender();
    bool send(const TouchEvent* request);

private:
    void doSend(const TouchEvent touch);

    const AndroidConsoleAgents* const mAgents;
    std::unordered_set<int> mUsedSlots;
    Looper* mLooper;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
