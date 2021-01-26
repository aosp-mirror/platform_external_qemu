package com.android.emulator.ui;

import com.android.emulator.control.ImageFormat;
import com.android.emulator.control.ImageTransport;
import com.android.emulator.utils.BufferedScreenshotRGB888;

import javax.swing.*;
import java.awt.*;
import java.awt.image.MemoryImageSource;
import java.io.File;
import java.io.IOException;
import java.net.URLDecoder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.EnumSet;

import static com.android.emulator.ui.EmulatorContainer.COLOR_FORMAT;

public class EmulatorPanel extends Canvas {
    public native void drawScreenshot(Graphics g, byte[] pixels, int width, int height);

    static {
        try {
            String path = EmulatorPanel.class.getProtectionDomain()
                    .getCodeSource()
                    .getLocation()
                    .getPath();
            path = Paths.get(URLDecoder.decode(path, "UTF-8")).getParent().getParent().toString();
            path += "/jni/" + System.mapLibraryName("nativecanvas");
            System.out.println(path);
            File lib = new File(path);
            System.load(lib.getAbsolutePath());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    private com.android.emulator.control.Image mEmuImg;
    private Image img;
    // For mmaped-based streaming
    private FileChannel fileChannel;
    private MappedByteBuffer mappedByteBuffer;
    BufferedScreenshotRGB888 bufferedScreenshotRGB888;
    private MemoryImageSource cachedImageSource;
    private ImageTransport.TransportChannel transportChannel;
    private String transportHandle;
    private Dimension dimension;

    public EmulatorPanel(int width, int height, ImageTransport.TransportChannel transportChannel, String transportHandle) {
        this.transportChannel = transportChannel;
        this.transportHandle = transportHandle;
        dimension = new Dimension(width, height);
        setPreferredSize(dimension);
        setMinimumSize(dimension);
        setMaximumSize(dimension);
        setSize(dimension);

        if (ImageTransport.TransportChannel.MMAP.equals(transportChannel)) {
            try {
                fileChannel = (FileChannel) Files.newByteChannel(Path.of(transportHandle),
                        EnumSet.of(StandardOpenOption.CREATE, StandardOpenOption.READ, StandardOpenOption.WRITE));
                bufferedScreenshotRGB888 = new BufferedScreenshotRGB888(width, height);
                mappedByteBuffer = fileChannel.map(FileChannel.MapMode.READ_WRITE, 0, getImageSize());
            } catch (IOException e) {
                System.err.println("Unable to open mmap file=" + transportHandle);
                System.err.println(e.getMessage());
            }
        }
    }

    public void setImageFromScreenshot(com.android.emulator.control.Image image) {
        if (ImageTransport.TransportChannel.MMAP.equals(transportChannel)) {
            onNewMmapedImage(image);
        } else {
            onNewGrpcImage(image);
        }
        repaint();
    }

    public void onNewMmapedImage(com.android.emulator.control.Image image) {
        // array() doesn't work; direct buffers must be accessed in native code
        byte[] imageBytes = mappedByteBuffer.array();
        System.out.println("setting pixels imageBytes.size=" + imageBytes.length);
        bufferedScreenshotRGB888.setPixels(imageBytes);
        System.out.println("getting image");
        this.img = bufferedScreenshotRGB888.getImage();
        System.out.println("got image");
    }

    public void onNewGrpcImage(com.android.emulator.control.Image image) {
//        MemoryImageSource imageSource = cachedImageSource;
//        if (imageSource == null) {
//            Screenshot screenshot = new Screenshot(image);
//            imageSource = new MemoryImageSource(
//                    image.getFormat().getWidth(),
//                    image.getFormat().getHeight(),
//                    screenshot.getPixels(),
//                    0,
//                    image.getFormat().getWidth());
//            imageSource.setAnimated(true);
//            this.img = createImage(imageSource);
//            cachedImageSource = imageSource;
//        }
//        else {
//            Screenshot screenshot = new Screenshot(image);
//            imageSource.newPixels(screenshot.getPixels(), ColorModel.getRGBdefault(), 0,
//                    screenshot.getWidth());
//        }
        mEmuImg = image;
    }

    @Override
    public void update(Graphics g) {
        // The update() method first clears the canvas before redrawing, which creates flickering. So we
        // remove the clearing and just redraw.
        if (isShowing()) paint(g);
    }

    @Override
    public void paint(Graphics g) {
//        g.drawImage(img, 0, 0, null);
        if (mEmuImg == null) {
            return;
        }
        long start = System.currentTimeMillis();
        drawScreenshot(g, mEmuImg.getImage().toByteArray(), dimension.width, dimension.height);
        long duration = System.currentTimeMillis() - start;
        System.out.println("drawScreenschot duration " + duration + " ms");
    }

    private int getImageSize() {
        int colorFormatSize = 0;

        if (ImageFormat.ImgFormat.RGB888.equals(COLOR_FORMAT)) {
            colorFormatSize = 3;
        } else if (ImageFormat.ImgFormat.RGBA8888.equals(COLOR_FORMAT)) {
            colorFormatSize = 4;
        }
        return dimension.height * dimension.width * colorFormatSize;
    }
}
