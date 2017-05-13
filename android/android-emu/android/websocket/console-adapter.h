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
    virtual void send(Receiver* from, Topic t, std::string msg) override;
    virtual std::string id() { return EMULATOR_RECEIVE; };
    void publish(std::string msg);

private:
    ControlClient mEmulator;
    Pubsub* mPubsub;
    std::string mResponse;
};

}  // namespace base
}  // namespace android
