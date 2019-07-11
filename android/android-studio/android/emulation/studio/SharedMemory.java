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

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.file.FileSystems;

/**
 * Enables reading of a Shared Memory handle.
 */
public final class SharedMemory {
    // Load lib.
    static {
        NativeLibLoader.load("libandroid-studio");
    }

    private final int mSize;
    private final long mHandle;

    /**
     * Map the given shared memory region into this process. This will enable reading from the
     * shared memory region.
     *
     * @param handle The name of the shared memory region
     * @param size The size of the region to be mapped
     */
    public SharedMemory(String handle, int size) throws RuntimeException {
        mSize = size;
        mHandle = openMemoryHandle(handle, size);
        if (mHandle == 0) {
            throw new RuntimeException("Unable to connect to shared memory region: " + handle);
        }
    }

    /**
     * Reads from the shared memory at the given offset into this array. The
     * contents of the shared memory region is copied into the destination buffer.
     *
     * @param dest The destination array.
     * @param offset The starting offset in bytes from where we want to read.
     * @return The destination array.
     */
    public byte[] read(byte[] dest, int offset) {
        readMemoryB(mHandle, dest, offset);
        return dest;
    }

    /**
     * Reads from the shared memory at offset 0 into this array. The
     * contents of the shared memory region is copied into the destination buffer.
     *
     * @param dest The destination array.
     * @return The destination array.
     */
    public byte[] read(byte[] dest) {
        return read(dest, 0);
    }

    /**
     * Reads from the shared memory at the given offset into this array. The
     * contents of the shared memory region is copied into the destination buffer.
     * Beware that the offset is in bytes. e.g. dest[1] is at an offset of 4.
     *
     * @param dest The destination array.
     * @param offset The starting offset in bytes from where we want to read.
     * @return The destination array.
     */
    public int[] read(int[] dest, int offset) {
        readMemoryI(mHandle, dest, offset);
        return dest;
    }

    /**
     * Reads from the shared memory at offset 0 into this array. The
     * contents of the shared memory region is copied into the destination buffer.
     *
     * @param dest The destination array.
     * @return The destination array.
     */
    public int[] read(int[] dest) {
        return read(dest, 0);
    }

    /**
     * Reads the whole memory region into a byte array.
     *
     * @return A new array containing all the shared bytes.
     */
    public byte[] read() {
        byte[] copy = new byte[mSize];
        return read(copy);
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        close(mHandle);
    }

    private native long openMemoryHandle(String handle, int size);
    private native boolean readMemoryB(long handle, byte[] destination, int offset);
    private native boolean readMemoryI(long handle, int[] destination, int offset);
    private native void close(long handle);
}
