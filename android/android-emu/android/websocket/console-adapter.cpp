#include "android/websockets/console-adapter.h"

using namespace android::base;

static void console_adapter_publish(ControlClient client, const char* msg, int cMsg) {
  ConsoleAdapter *ca = reinterpret_cast<ConsoleAdapter*>(client->opaque);
  ca->publish(std::string(msg, cMsg));
  ca->mPubsub->publish(EMULATOR_TOPIC, std::string(msg, cMsg));
}

void ConsoleAdapter::send(std::string msg) {
    memset(mEmulator->buff, 0, 4096);
    memset(mEmulator->buff, data.c_str(), std::min(4096, (int) data.size()));
    mEmulator->buff_len = data.size();
    control_client_do_command(mEmulator);
}

void ConsoleAdapter::publish(std::string msg) {
  mPubsub->publish(msg);
}

ConsoleAdapter::ConsoleAdapter(Pubsub* pubsub) : mPubsub(pubsub) {
    mEmulator = control_client_create_empty();
    mEmulator->opaque = this;
    mEmulator->write = WSConsole_send;
}

