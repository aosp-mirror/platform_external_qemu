package com.android.emulator.ui;

import com.android.emulator.control.ImageTransport;
import com.android.emulator.utils.Cancelable;
import com.android.emulator.utils.EmptyStreamObserver;
import com.android.emulator.utils.EmulatorController;
import com.android.emulator.control.ImageFormat;
import com.android.emulator.utils.Screenshot;
import org.jetbrains.annotations.NotNull;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.image.ColorModel;
import java.awt.image.MemoryImageSource;
import java.util.logging.Level;
import java.util.logging.Logger;

public class EmulatorContainer {
    static Logger logger = Logger.getLogger(EmulatorContainer.class.getName());
    // Change between TRANSPORT_CHANNEL_UNSPECIFIED and MMAP
    static final ImageTransport.TransportChannel TRANSPORT_CHANNEL =
            ImageTransport.TransportChannel.TRANSPORT_CHANNEL_UNSPECIFIED;
    // TODO: mac/windows support
    static final String TRANSPORT_HANDLE = "file:////var/tmp/emu-client-12345.mmap";
    static final int IMG_WIDTH = 1080 / 3;
    static final int IMG_HEIGHT = 1920 / 3;
    static final int WINDOW_WIDTH = IMG_WIDTH + 20;
    static final int WINDOW_HEIGHT = IMG_HEIGHT + 20;

    private ImagePanel rootPanel;
    private EmulatorController emulatorController;
    private Cancelable screenshotFeed;
    private ScreenshotReceiver screenshotReceiver;
    private MemoryImageSource cachedImageSource;

    public EmulatorContainer() {
        rootPanel = new ImagePanel();
        rootPanel.addMouseListener(new MouseListener() {
            @Override
            public void mouseClicked(MouseEvent e) {
                logger.log(Level.FINEST, "mouseClicked x=" + e.getX() + "y=" + e.getY());
            }

            @Override
            public void mousePressed(MouseEvent e) {
                logger.log(Level.FINEST, "mousePressed x=" + e.getX() + "y=" + e.getY());
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                logger.log(Level.FINEST, "mouseReleased x=" + e.getX() + "y=" + e.getY());
            }

            @Override
            public void mouseEntered(MouseEvent e) {
                logger.log(Level.FINEST, "mouseEntered x=" + e.getX() + "y=" + e.getY());
            }

            @Override
            public void mouseExited(MouseEvent e) {
                logger.log(Level.FINEST, "mouseExited x=" + e.getX() + "y=" + e.getY());
            }
        });

        rootPanel.addMouseMotionListener(new MouseMotionListener() {
            @Override
            public void mouseDragged(MouseEvent e) {
                logger.log(Level.FINEST, "mouseDragged x=" + e.getX() + "y=" + e.getY());
            }

            @Override
            public void mouseMoved(MouseEvent e) {
                logger.log(Level.FINEST, "mouseMoved x=" + e.getX() + "y=" + e.getY());
            }
        });

        emulatorController = new EmulatorController();
        emulatorController.connect();

        System.out.println("Using transport_channel=" + TRANSPORT_CHANNEL + " handle=" + TRANSPORT_HANDLE);
        ImageFormat imageFormat = ImageFormat.newBuilder()
                .setFormat(ImageFormat.ImgFormat.RGB888)
                .setWidth(IMG_WIDTH)
                .setHeight(IMG_HEIGHT)
                .setTransport(
                        ImageTransport.newBuilder()
                                .setChannel(TRANSPORT_CHANNEL)
                                .setHandle(TRANSPORT_HANDLE)
                                .build())
                .build();

        screenshotReceiver = new ScreenshotReceiver();
        screenshotFeed = emulatorController.streamScreenshot(imageFormat, screenshotReceiver);
    }

    private class ScreenshotReceiver extends EmptyStreamObserver<com.android.emulator.control.Image> {
        MemoryImageSource cachedImageSource;

        @Override
        public void onNext(@NotNull com.android.emulator.control.Image response) {
            long latency = System.currentTimeMillis() - response.getTimestampUs() / 1000;
//            logger.log(Level.FINEST, "Screenshot " + response.getSeq() + " "
            System.out.println("Screenshot " + response.getSeq() + " "
                    + response.getFormat().getWidth() + "x" + response.getFormat().getHeight()
                    + " " + response.getFormat().getRotation().getRotation()
                    + " " + latency + " ms latency");

            if (screenshotReceiver != this) {
                return; // This screenshot feed has already been cancelled.
            }

            if (response.getFormat().getWidth() == 0 || response.getFormat().getHeight() == 0) {
                return; // Ignore empty screenshot.
            }
            rootPanel.setImageFromScreenshot(response);
        }
    }

    public void setImageFromScreenshot(@NotNull com.android.emulator.control.Image image) {
    }

    class ImagePanel extends JPanel {
        private Image img;

        public ImagePanel(String img) {
            this(new ImageIcon(img).getImage());
        }

        public ImagePanel(Image img) {
            this.img = img;
            Dimension size = new Dimension(img.getWidth(null), img.getHeight(null));
            setPreferredSize(size);
            setMinimumSize(size);
            setMaximumSize(size);
            setSize(size);
            setLayout(null);
        }

        public ImagePanel() {
            Dimension size = new Dimension(IMG_WIDTH, IMG_HEIGHT);
            setPreferredSize(size);
            setMinimumSize(size);
            setMaximumSize(size);
            setSize(size);
            setLayout(null);
        }

        public void setImageFromScreenshot(com.android.emulator.control.Image image) {
            MemoryImageSource imageSource = cachedImageSource;
            if (imageSource == null) {

            } else {
                imageSource.newPixels();
            }
            if (imageSource == null) {
                Screenshot screenshot = new Screenshot(image);
                imageSource = new MemoryImageSource(
                        image.getFormat().getWidth(),
                        image.getFormat().getHeight(),
                        screenshot.getPixels(),
                        0,
                        image.getFormat().getWidth());
                imageSource.setAnimated(true);
                this.img = createImage(imageSource);
                cachedImageSource = imageSource;
            }
            else {
                Screenshot screenshot = new Screenshot(image);
                imageSource.newPixels(screenshot.getPixels(), ColorModel.getRGBdefault(), 0,
                        screenshot.getWidth());
            }
            repaint();
        }
        public void paintComponent(Graphics g) {
            g.drawImage(img, 0, 0, null);
        }

    }

    public static void main(String[] args) {
        logger.setLevel(Level.FINEST);

        JFrame frame = new JFrame("HelloWorld");
        frame.setContentPane(new EmulatorContainer().rootPanel);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);
    }
}
