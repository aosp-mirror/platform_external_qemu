// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
package android.emulation.studio.net;

import android.emulation.studio.net.EmulatorListener.ConnectionState;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Optional;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Future;
import java.util.function.Predicate;
import java.util.logging.Logger;
import java.util.stream.Stream;

/**
 * Connects to the emulator over the telnet port.
 *
 * It will authenticate by sending the token that the emulator requests.
 * The client *must* have access to the token file that the emulator requests.
 *
 * Events will be send out for received messages and state changes
 *
 * @see EmulatorListener
 * @author Erwin Jansen (jansene@google.com)
 */
public class EmulatorTelnetClient {

  private static final Logger LOGGER = Logger.getLogger(EmulatorTelnetClient.class.getName());
  private static final String AUTH_REQUEST = "Android Console: you can find your <auth_token> in";

  private final InetSocketAddress mHostAddress;
  private AsynchronousSocketChannel client;
  private CopyOnWriteArrayList<EmulatorListener> listeners = new CopyOnWriteArrayList<>();
  private boolean mAuthorized;

  public EmulatorTelnetClient(int port)  {
    this.mHostAddress = new InetSocketAddress("localhost", port);
  }

  private static <T> Predicate<T> dropUntil(Predicate<T> predicate) {
    boolean[] keep = { false }; // Magically keep state..
    return t -> keep[0] || (keep[0] = predicate.test(t));
  }

  /**
   * Asynchronously connect to the android emulator.
   *
   * A connection event will be send once an authentication token has been send.
   * @throws IOException
   */
  public void connect() throws IOException {
    this.client = AsynchronousSocketChannel.open();
    this.client.connect(mHostAddress, null, new CompletionHandler<Void, Object>() {
      @Override
      public void completed(Void aVoid, Object o) {
        LOGGER.info("Connected to " + mHostAddress);
        read();
      }

      @Override
      public void failed(Throwable throwable, Object o) {
        connectionStateChanged(ConnectionState.Disconnected);
      }
    });
    connectionStateChanged(ConnectionState.Connecting);
  }

  private void read() {
    ByteBuffer buffer = ByteBuffer.allocate(4096);
    client.read(buffer, null,
        new CompletionHandler<Integer, Object>() {
          @Override
          public void completed(Integer result, Object attachment) {

            if (result < 0) {
              connectionStateChanged(ConnectionState.Disconnected);
            } else {
              String msg = new String(buffer.array(), 0, result);
              messageReceived(msg);
              read();
            }
          }

          @Override
          public void failed(Throwable e, Object attachment) {
            connectionStateChanged(ConnectionState.Disconnected);
          }
        });
  }

  private void connectionStateChanged(ConnectionState state) {
    LOGGER.info("Changing state to " + state);
    if (state != ConnectionState.Connected) {
      mAuthorized = false;
    }
    for (EmulatorListener l : listeners) {
      l.stateChange(state);
    }
  }

  private void messageReceived(String msg) {
    LOGGER.fine("Received: " + msg);

    // Clean up the incoming results.
    Stream<String> lines = Arrays.stream(msg.split("\n")).map(x -> x.trim());
    if (mAuthorized) {
      // Forward it as we have send an auth token.
      for (EmulatorListener l : listeners) {
         lines.forEach(x -> {l.messageReceived(x);});
      }
    } else {
      // Wait until the auth string comes in.
      // Well java 9: introduces dropWhile, which we don't have in Java 8..
      Optional<String> tokenFile = lines.filter(dropUntil(x ->
                  x.contains(AUTH_REQUEST))).skip(1).findFirst();
      if (tokenFile.isPresent()) {
        auth(tokenFile.get());
      }
    }
  }

  public void addEmulatorListener(EmulatorListener l) {
    listeners.add(l);
  }

  public void removeEmulatorListener(EmulatorListener l) {
    listeners.remove(l);
  }

  private void auth(String fileName) {
    try {
      String fname = fileName.replace("'", "");
      String token = new String(Files.readAllBytes(Paths.get(fname)));
      send(String.format("auth %s", token));
      mAuthorized = true;
      connectionStateChanged(ConnectionState.Connected);
      LOGGER.fine("Sending token dropUntil " + fname);
    } catch (IOException ioe) {
      LOGGER.severe("Unable to send authorization token due to: " + ioe);
    }
  }

  /**
   * Sends the given message to the emulator. A newline terminator will be added.
   *
   * @param message The message to send.
   * @return A future with the number of bytes that were send.
   */
  public Future<Integer> send(String message) {
    LOGGER.fine("Sending " + message);
    byte[] byteMsg = String.format("%s\n", message).getBytes();
    ByteBuffer buffer = ByteBuffer.wrap(byteMsg);
    return client.write(buffer);
  }
}
