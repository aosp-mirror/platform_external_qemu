package com.android.emulator.utils

import com.android.emulator.control.Image
import java.awt.image.BufferedImage

// Similar to Screenshot class, but using a buffer that is allocated from
// java.awt.Image.BufferedImage
class BufferedScreenshotRGB888(width: Int, height: Int) {
    val image: BufferedImage = BufferedImage(width, height, BufferedImage.TYPE_INT_RGB)
    val width: Int = width
    val height: Int = height

    fun setPixels(imageBytes: ByteArray) {
        System.out.println("image.getRGB calling")
        val pixels = image.getRGB(0,0,width,height,null,0,width)
        System.out.println("image.getRGB finished pixels.length=" + pixels.size + " imageBytes.size=" + imageBytes.size)
        val byteIterator = imageBytes.iterator()
        val alpha = 0xFF shl 24
        for (i in pixels.indices) {
            val red = byteIterator.nextByte().toInt() and 0xFF
            val green = byteIterator.nextByte().toInt() and 0xFF
            val blue = byteIterator.nextByte().toInt() and 0xFF
            pixels[i] = alpha or (red shl 16) or (green shl 8) or blue
        }
    }

//    fun getImage(): java.awt.Image {
//        return image
//    }
}
