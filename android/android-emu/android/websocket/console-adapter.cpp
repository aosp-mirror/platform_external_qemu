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
    mEmulator = control_client_create_empty();
    mEmulator->opaque = this;
    mEmulator->write = console_adapter_publish;
    mPubsub->subscribe(this, EMULATOR_RECEIVE);
}

void ConsoleAdapter::send(std::string msg) {
    memset(mEmulator->buff, 0, MAX_CONSOLE_MSG_LENGTH);
    memcpy(mEmulator->buff, msg.c_str(),
           std::min(MAX_CONSOLE_MSG_LENGTH, (int)msg.size()));
    mEmulator->buff_len = msg.size();
    control_client_do_command(mEmulator);
}

void ConsoleAdapter::publish(std::string msg) {
    mPubsub->publish(EMULATOR_BROADCAST, msg);
}

