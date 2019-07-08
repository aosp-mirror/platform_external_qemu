package android.emulation.studio;

import javax.swing.*;
import java.awt.*;
import java.awt.image.MemoryImageSource;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * A View of the current emulator.
 */
public class EmulatorView extends JPanel {
  public static final String DEFAULT_SHARED_MEMORY_REGION = "videmulator5554";
  private static final int VIDEO_INFO_STRUCT_SIZE = 20;

  private final MemoryImageSource mImageSource;
  private final byte[] mVideoInfo;
  private final ByteBuffer mVideoInfoView;
  private final int[] mImgBuffer;
  private final SharedMemory mSharedMemory;
  private Image mImage;
  private int mPrevFrame;

  /**
   * Creates a view of the emulator video by connecting to the "videmulator5554" handle.
   */
   public EmulatorView() {
    this(DEFAULT_SHARED_MEMORY_REGION);
  }

  /**
   * Creates a view of the emulator video by connecting to the given shared memory handle.
   *
   * @param handle The name of the shared memory handle.
   */
  public EmulatorView(String handle) {
    this(handle, Executors.newSingleThreadScheduledExecutor());
  }

  /**
   * Creates a view of the emulator video by connecting to the given shared memory handle.
   *
   * @param handle The name of the shared memory handle.
   * @param service The executor service used to schedule updates at max fps per second.
   */
  public EmulatorView(String handle, ScheduledExecutorService service) {
    SharedMemory videoInfo = new SharedMemory(handle, VIDEO_INFO_STRUCT_SIZE);
    mVideoInfo = videoInfo.read();
    mVideoInfoView = ByteBuffer.wrap(mVideoInfo);
    mVideoInfoView.order(ByteOrder.LITTLE_ENDIAN);
    int width = getVideoWidth();
    int height = getVideoHeight();
    mSharedMemory = new SharedMemory(handle, width * height * 4 + VIDEO_INFO_STRUCT_SIZE);
    mImgBuffer = new int[width * height];
    mSharedMemory.read(mImgBuffer, VIDEO_INFO_STRUCT_SIZE);
    mImageSource = new MemoryImageSource(width, height, mImgBuffer, 0, width);
    mImageSource.setAnimated(true);
    mImage = createImage(mImageSource);

    service.scheduleAtFixedRate(
            () -> {
              this.updateImageSource();
            },
            0,
            1000 / getVideoFps(),
            TimeUnit.MICROSECONDS);
  }

  /**
   * The width of the emulator video window.
   *
   * @return
   */
  public int getVideoWidth() {
    return mVideoInfoView.getInt(0);
  }

  /**
   * The height of the emulator video window.
   *
   * @return The height.
   */
  public int getVideoHeight() {
    return mVideoInfoView.getInt(4);
  }

  /**
   * The desired max framerate at which the emulator delivers frames.
   *
   * @return The maximum frame rate.
   */
  public int getVideoFps() {
    return mVideoInfoView.getInt(8);
  }

  /**
   *
   * @return
   */
  public int getVideoFrame() {
    return mVideoInfoView.getInt(12);
  }

  /**
   * The timestamp in microseconds, when this image was generated in the emulator.
   *
   * @return Timestamp in microseconds.
   */
  public int getVideoTimestamp() {
    return mVideoInfoView.getInt(16);
  }

  private void updateImageSource() {
    mSharedMemory.read(mVideoInfo, 0);
    int frame = getVideoFrame();
    if (frame != mPrevFrame) {
      mSharedMemory.read(mImgBuffer, VIDEO_INFO_STRUCT_SIZE);
      mImageSource.newPixels();
      repaint();
      mPrevFrame = frame;
    }
  }

  @Override
  protected void paintComponent(Graphics g) {
    super.paintComponent(g);
    g.drawImage(mImage, 0, 0, getWidth(), getHeight(), null);
  }
}

