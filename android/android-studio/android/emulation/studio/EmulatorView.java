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
package android.emulation.studio;

import java.awt.*;
import java.awt.geom.AffineTransform;
import java.awt.image.MemoryImageSource;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import java.util.logging.Logger;
import javax.swing.*;

/**
 * A View of the current emulator.
 *
 * <p>This view relies on a SharedMemory region where the ARGB pixels can be found. The view will
 * update itself by checking if the frame counter in the shared memory region has changed.
 *
 * <p>The SharedMemory region should have the following layout:
 *
 * <p>struct VideoInfo { uint32_t width; uint32_t height; uint32_t fps; // Target framerate.
 * uint32_t frameNumber; // Frame number. uint64_t tsUs; // Timestamp when this frame was received.
 * uint8_t[width*height*4] // pixels; };
 *
 * <p>Note: This panel does not keep the aspect ratio. You can use the preferredsize to calculate
 * the proper aspect ratio.
 *
 * @author Erwin Jansen (jansene@google.com)
 */
public class EmulatorView extends JPanel {

  public static final String DEFAULT_SHARED_MEMORY_REGION = "videmulator5554";
  public static final int RIGHT_ANGLE = 90;
  public static final int FULL_ANGLE = 360;
  private static final Logger LOGGER = Logger.getLogger(EmulatorView.class.getName());
  private static final int VIDEO_INFO_STRUCT_SIZE = 20;
  private final MemoryImageSource mImageSource;
  private final byte[] mVideoInfo;
  private final ByteBuffer mVideoInfoView;
  private final int[] mImgBuffer;
  private final SharedMemory mSharedMemory;
  private Image mImage;
  private int mPrevFrame = -1;
  private int mRotation = 0;

  /**
   * Creates a view of the emulator video by connecting to the given shared memory handle. A handle
   * usually follows the following convetion videmulatorXXXX where XXXX is the port number.
   *
   * <p>For example videmulator5554 will be the most commonly used handle.
   *
   * <p>A background task will be scheduled every 1000/FPS seconds to see if a new frame has
   * arrived.
   *
   * @param handle The name of the shared memory handle.
   */
  public EmulatorView(String handle) {
    this(handle, Executors.newSingleThreadScheduledExecutor());
  }

  /**
   * Creates a view of the emulator video by connecting to the given shared memory handle.
   *
   * <p>A background task will be scheduled every 1000/FPS seconds to see if a new frame has arrived
   * on the given executor services.
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
        TimeUnit.MILLISECONDS);
  }

  /**
   * The width of the emulator video window.
   *
   * @return The width.
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
   * Gets the preferred size, this depends on the rotatation of the emulator.
   *
   * @return
   */
  @Override
  public Dimension getPreferredSize() {
    if (mRotation == 1 || mRotation == 2) {
      return new Dimension(getVideoHeight(), getVideoWidth());
    }
    return new Dimension(getVideoWidth(), getVideoHeight());
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
   * Gets the number of the latest received video frame. This is a monotonically increasing number.
   * A repaint will be requested when this number changes.
   *
   * @return The frame number.
   */
  public int getVideoFrame() {
    return mVideoInfoView.getInt(12);
  }

  /**
   * The timestamp in microseconds when this image was generated in the emulator.
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

  /**
   * Gets the current rotation quadrant.
   *
   * @return The quadrant used to rotate the view to.
   */
  public int getRotationQuadrant() {
    return mRotation;
  }

  /** Rotates the view to the next quadrant. */
  public void rotate() {
    mRotation = (mRotation + 1) % 4;
  }

  @Override
  protected void paintComponent(Graphics g) {
    super.paintComponent(g);
    Graphics2D g2d = (Graphics2D) g;
    AffineTransform Tx = new AffineTransform();

    int h = getVideoHeight();
    int w = getVideoWidth();
    double ow = getWidth();
    double oh = getHeight();

    // Make sure the image is scaled properly.
    switch (mRotation) {
      case 0:
        Tx.setToScale(ow /w, oh/h);
        break;
      case 2:
        Tx.setToScale(ow /w, oh/h);
        Tx.translate(w / 2, h / 2);
        Tx.rotate(Math.PI);
        Tx.translate(-w / 2, -h / 2);
        break;
      case 1:
      case 3:
        Tx.setToScale(ow /h, oh/w);
        Tx.translate(h / 2, w / 2); // Center back..
        Tx.rotate(mRotation * Math.PI / 2.0); // S2: rotate around anchor
        Tx.translate(-w / 2, -h / 2);
        break;
    }

    g2d.drawImage(mImage, Tx, null);
  }
}
