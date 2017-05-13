#include "android/websocket/websocketserver.h"
#include "android/base/threads/Thread.h"
#include "android/console.h"
#include <unordered_map>

typedef std::unordered_map<char, int> NameToClientMap;
typedef std::unordered_map<int, char> ClientToNameMap;

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
};

