#include "android/websocket/console-adapter.h"
using namespace android::base;

extern "C" {
  static void console_adapter_publish(ControlClient client,
                                      const char* msg,
                                      int cMsg) {
      ConsoleAdapter* ca = reinterpret_cast<ConsoleAdapter*>(client->opaque);
      ca->publish(std::string(msg, cMsg));
  }
}

ConsoleAdapter::ConsoleAdapter(Pubsub* pubsub) : mPubsub(pubsub) {
    mResponse = "";
    mEmulator = control_client_create_empty();
    mEmulator->opaque = this;
    mEmulator->write = console_adapter_publish;
    mPubsub->subscribe(this, EMULATOR_RECEIVE);
}

void ConsoleAdapter::send(Receiver* from, Topic t, std::string msg) {
    memset(mEmulator->buff, 0, MAX_CONSOLE_MSG_LENGTH);
    memcpy(mEmulator->buff, msg.c_str(),
           std::min(MAX_CONSOLE_MSG_LENGTH, (int)msg.size()));
    mEmulator->buff_len = msg.size();
    fprintf(stderr, "%s Received: [%s]\n", __func__, mEmulator->buff);
    control_client_do_command(mEmulator);
}

void ConsoleAdapter::publish(std::string msg) {
    mResponse.append(msg);
    if (msg.compare(0, 2, "OK") == 0 || msg.compare(0, 2, "KO") == 0) {
        mPubsub->publish(this, EMULATOR_BROADCAST, mResponse);
        mResponse = "";
    }
}

