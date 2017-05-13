#include "android/websocket/pubsub.h"

#define EMULATOR_TOPIC "emulator_console"
namespace android {
namespace base {

class ConsoleAdapter : public Receiver {

 public:
  ConsoleAdapter(Pubsub* pubsub);
  void send(std::string msg) override;

  ControlClient mEmulator;
  Pubsub* mPubsub;
  
};

extern "C" {
  void console_adapter_publish(ControlClient client, const char* msg, int cMsg);
}

}
}
