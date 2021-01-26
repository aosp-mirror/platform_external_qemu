package com.android.emulator.utils

import com.android.emulator.control.Image
import com.google.protobuf.ByteString

class Screenshot(emulatorImage: Image) {
    val pixels: IntArray
    val width: Int
    val height: Int

    init {
        val format = emulatorImage.format
        width = format.width
        height = format.height
        pixels = getPixels(emulatorImage.image, width, height)
    }

    private fun getPixels(imageBytes: ByteString, width: Int, height: Int): IntArray {
        val pixels = IntArray(width * height)
        val byteIterator = imageBytes.iterator()
        val alpha = 0xFF shl 24
        for (i in pixels.indices) {
            val red = byteIterator.nextByte().toInt() and 0xFF
            val green = byteIterator.nextByte().toInt() and 0xFF
            val blue = byteIterator.nextByte().toInt() and 0xFF
            pixels[i] = alpha or (red shl 16) or (green shl 8) or blue
        }
        return pixels
    }
}