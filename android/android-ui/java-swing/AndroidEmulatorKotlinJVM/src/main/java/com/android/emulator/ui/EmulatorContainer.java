package com.android.emulator.ui;

import com.android.emulator.control.ImageTransport;
import com.android.emulator.utils.*;
import com.android.emulator.control.ImageFormat;
import org.jetbrains.annotations.NotNull;

import javax.swing.*;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.util.logging.Level;
import java.util.logging.Logger;

import static java.lang.System.exit;

public class EmulatorContainer {
    static Logger logger = Logger.getLogger(EmulatorContainer.class.getName());
    // Change between TRANSPORT_CHANNEL_UNSPECIFIED and MMAP
    static ImageTransport.TransportChannel TRANSPORT_CHANNEL =
            ImageTransport.TransportChannel.TRANSPORT_CHANNEL_UNSPECIFIED;
    // TODO: mac/windows support
    static final String TRANSPORT_HANDLE_PREFIX = "file:///";
    static String TRANSPORT_HANDLE = "/var/tmp/emu-client-12345.mmap";

    static int userWidth = 1080;
    static int userHeight = 1920;
    static final ImageFormat.ImgFormat COLOR_FORMAT = ImageFormat.ImgFormat.RGBA8888;

    private EmulatorPanel rootPanel;
    private EmulatorController emulatorController;
    private Cancelable screenshotFeed;
    private ScreenshotReceiver screenshotReceiver;

    public EmulatorContainer() {
        rootPanel = new EmulatorPanel(userWidth, userHeight, TRANSPORT_CHANNEL, TRANSPORT_HANDLE);
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

        System.out.println("Using resolution=" + userWidth + "x" + userHeight + " " +
                "transport_channel=" + TRANSPORT_CHANNEL + " handle=" + TRANSPORT_HANDLE);
        ImageFormat imageFormat = ImageFormat.newBuilder()
                .setFormat(COLOR_FORMAT)
                .setWidth(userWidth)
                .setHeight(userHeight)
                .setTransport(
                        ImageTransport.newBuilder()
                                .setChannel(TRANSPORT_CHANNEL)
                                .setHandle(TRANSPORT_HANDLE_PREFIX + TRANSPORT_HANDLE)
                                .build())
                .build();

        screenshotReceiver = new ScreenshotReceiver();
        screenshotFeed = emulatorController.streamScreenshot(imageFormat, screenshotReceiver);
    }

    private class ScreenshotReceiver extends EmptyStreamObserver<com.android.emulator.control.Image> {
        @Override
        public void onNext(@NotNull com.android.emulator.control.Image response) {
            long latency = System.currentTimeMillis() - response.getTimestampUs() / 1000;
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


    public static void main(String[] args) {
        logger.setLevel(Level.FINEST);

        if (!getCmdArguments(args)) {
            exit(1);
        }
        JFrame frame = new JFrame("HelloWorld");
        frame.add(new EmulatorContainer().rootPanel);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);
    }

    private static boolean getCmdArguments(String[] args) {
        if (args.length < 2) {
            printHelp("Invalid number of arguments");
            return false;
        }

        String[] resolution = args[0].split("x");
        if (resolution.length != 2) {
            printHelp("Invalid resolution");
            return false;
        }

        try {
            userWidth = Integer.parseInt(resolution[0]);
            userHeight = Integer.parseInt(resolution[1]);
        } catch (NumberFormatException e) {
            printHelp("Unable to parse resolution");
            return false;
        }

        if ("grpc".equals(args[1])) {
            TRANSPORT_CHANNEL = ImageTransport.TransportChannel.TRANSPORT_CHANNEL_UNSPECIFIED;
        } else if ("mmap".equals(args[1])) {
            if (args.length < 3) {
                printHelp("mmap transport requires a file path argument");
                return false;
            }
            TRANSPORT_CHANNEL = ImageTransport.TransportChannel.MMAP;
            TRANSPORT_HANDLE = args[2];
        }

        return true;
    }

    private static void printHelp(String err) {
        System.err.println("ERROR: " + err);
        System.err.println("Usage: android-emulator-client <WxH> <grpc|mmap <file>>\n");
        System.err.println("Examples:");
        System.err.println("\t$ android-emulator-client 1080x1920 grpc");
        System.err.println("\t$ android-emulator-client 2160x3840 mmap /var/tmp/emu-client-12345.mmap");
    }
}
