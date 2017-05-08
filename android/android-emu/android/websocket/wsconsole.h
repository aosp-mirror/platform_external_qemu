#include "android/websocket/websocketserver.h"
#include "android/base/threads/Thread.h"
#include "android/console.h"

class WSConsole : public WebSocketServer, public android::base::Thread 
{
    DISALLOW_COPY_ASSIGN_AND_MOVE(WSConsole);
public: 
    WSConsole( int port );
    ~WSConsole( );
    virtual void onConnect(    int socketID                        );
    virtual void onMessage(    int socketID, const string& data    );
    virtual void onDisconnect( int socketID                        );
    virtual void   onError(    int socketID, const string& message );
    virtual intptr_t main() {
      fprintf(stderr, "%s\n", __func__);
      this->run();
      return 0;
    }

    ControlClient me;
};

extern "C" {
  void WSConsole_send(ControlClient client, const char* msg, int cMsg);
}
// Create new instance, but does not start it.
//     MyThread* thread = new MyThread();

    // Start the thread.
    // thread->start();


