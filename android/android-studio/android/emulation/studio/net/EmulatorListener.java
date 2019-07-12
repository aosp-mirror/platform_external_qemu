package android.emulation.studio.net;

import java.util.EventListener;

public interface EmulatorListener extends EventListener {

  enum ConnectionState {Connected, Disconnected, Connecting}

  void messageReceived(String message);

  void stateChange(ConnectionState state);

}
