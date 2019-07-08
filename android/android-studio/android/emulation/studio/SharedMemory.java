package android.emulation.studio;

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.file.FileSystems;

public class SharedMemory {
    // Load lib.
    static {
        String os = System.getProperty("os.name");
        String sdk_root = System.getenv().get("ANDROID_SDK_ROOT");
        String ext = ".so";
        if (sdk_root == null) {
            // Assume the .so is next to us.
            sdk_root = "lib64" + File.separator;
        } else {
            // Insert libs etc..
            sdk_root += File.separator + "emulator" + File.separator + "lib64" + File.separator;
        }
        if (os.startsWith("Windows")) {
            ext = ".dll";

        } else if (os.startsWith("Mac")) {
            ext = ".dylib";
        }

        System.load(
                FileSystems.getDefault()
                        .getPath(sdk_root, "libandroid-studio" + ext) // Dynamic link
                        .normalize()
                        .toAbsolutePath()
                        .toString());
    }

    private final int mSize;
    private final long mHandle;

    public SharedMemory(String handle, int size) throws RuntimeException {
        mSize = size;
        mHandle = openMemoryHandle(handle, size);
        if (mHandle == 0) {
            throw new RuntimeException("Unable to connect to shared memory region: " + handle);
        }
    }

    public byte[] read(byte[] dest, int offset) {
        readMemoryB(mHandle, dest, offset);
        return dest;
    }

    public byte[] read(byte[] dest) {
        return read(dest, 0);
    }

    public int[] read(int[] dest, int offset) {
        readMemoryI(mHandle, dest, offset);
        return dest;
    }

    public int[] read(int[] dest) {
        return read(dest, 0);
    }

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
