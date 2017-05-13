#include "android/console.h"
#include "android/websocket/pubsub.h"

#define EMULATOR_BROADCAST "emulator-console-out"
#define EMULATOR_RECEIVE "emulator"

namespace android {
namespace base {

/*
 * Adapts the control client (telnet interface) to
 * a pubsub system. The emulator is subscribed to:
 *
 * the emulator topic and will broadcast to the
 * emulator-console-out topic
 *
 */
class ConsoleAdapter : public Receiver {
    DISALLOW_COPY_ASSIGN_AND_MOVE(ConsoleAdapter);

public:
    ConsoleAdapter(Pubsub* pubsub);
    void send(std::string msg) override;
    void publish(std::string msg);

private:
    ControlClient mEmulator;
    Pubsub* mPubsub;
};

}  // namespace base
}  // namespace android
