package android.emulation.studio;

import android.emulation.studio.net.EmulatorListener;
import android.emulation.studio.net.EmulatorTelnetClient;
import java.awt.BorderLayout;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.io.IOException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;
import javax.swing.JLabel;
import javax.swing.JPanel;

/**
 * An EmulatorView that will forward clicks and key events to the emulator.
 *
 * Communication with the emulator happens over the telnet port, so the emulator must run locally.
 * This emulator view will configure the emulator to start sharing its display in a shared memory
 * region.
 *
 * The display will be activated as soon as the emulator has configured the shared memory region.
 * The view will attempt to reconnect if the emulator disappears.
 *
 * <i>Note: This requires the system-image to support generic events!</i>
 * <i>Note: The emulator does not provide a mechanism to inform us of rotation events.</i>
 *
 * @author Erwin Jansen (jansene@google.com)
 * @see EmulatorView
 * @see EmulatorTelnetClient
 */
public class InteractiveEmulatorView extends JPanel implements EmulatorListener {

  private static final Logger LOGGER = Logger.getLogger(InteractiveEmulatorView.class.getName());
  private final JLabel mLabel;
  private final ScheduledExecutorService mExecutorService;
  private EmulatorView mView;
  private EmulatorTelnetClient mClient;


  public InteractiveEmulatorView(int port) {
    this(port, Executors.newSingleThreadScheduledExecutor());
  }

  public InteractiveEmulatorView(int port, ScheduledExecutorService ses) {
    super(new BorderLayout());
    mExecutorService = ses;
    mClient = new EmulatorTelnetClient(port);
    mClient.addEmulatorListener(this);
    this.mLabel = new JLabel("Not connected to emulator on port: " + port);
    add(this.mLabel);

    connect();
  }

  private void sendClick(int x, int y, int button) {
    int emuX = (int) ((float) mView.getVideoWidth() / mView.getWidth() * x);
    int emuY = (int) ((float) mView.getVideoHeight() / mView.getHeight() * y);
    mClient.send(String.format("event mouse %d %d 0 %d", emuX, emuY, button));
  }

  private void activateVideoView(String handle) {
    this.mView = new EmulatorView(handle, mExecutorService);

    // Forward mouse & keyboard events.
    this.mView.addMouseMotionListener(new MouseMotionAdapter() {
      @Override
      public void mouseDragged(MouseEvent e) {
        sendClick(e.getX(), e.getY(), 1);
      }
    });
    this.mView.addMouseListener(new MouseAdapter() {
      @Override
      public void mousePressed(java.awt.event.MouseEvent e) {
        sendClick(e.getX(), e.getY(), 1);
      }

      @Override
      public void mouseReleased(java.awt.event.MouseEvent e) {
        sendClick(e.getX(), e.getY(), 0);
      }
    });
    this.mView.addKeyListener(new KeyAdapter() {
      @Override
      public void keyPressed(KeyEvent e) {
        mClient.send(KeyMapper.translate(e, true));
      }

      @Override
      public void keyReleased(KeyEvent e) {
        mClient.send(KeyMapper.translate(e, false));
      }
    });

    // We must be focusable to receive keyboard events.
    mView.setFocusable(true);

    // Activate view.
    add(mView);
    remove(mLabel);
    revalidate();
    repaint();
  }

  private void connect() {
    try {
      mClient.connect();
    } catch (IOException e) {
      // Too bad.. so sad.. We rely on the callbacks to initiate a reconnect.
      LOGGER.info("Connection failed " + e);
    }
  }

  @Override
  public void messageReceived(String message) {
    // We are waiting for the message from the webrtc engine
    // and ignore all the other messages.
    if (message.startsWith("videmulator")) {
      activateVideoView(message);
    }
  }

  /**
   * Rotate the emulator by 90 degrees.
   */
  public void rotate() {
    mClient.send("rotate");
    mView.rotate();
  }

  @Override
  public void stateChange(ConnectionState state) {
    if (state == ConnectionState.Disconnected) {
      add(mLabel);
      remove(mView);
      revalidate();
      repaint();

      // Attempt to reconnect a second from now.
      mExecutorService.schedule(() -> {
        connect();
      }, 1, TimeUnit.SECONDS);
    }

    if (state == ConnectionState.Connected) {
      // Start the webrtc module so screen sharing becomes active.
      // We will act upon the response by starting the view.
      mClient.send("screenrecord webrtc start");
    }
  }
}
