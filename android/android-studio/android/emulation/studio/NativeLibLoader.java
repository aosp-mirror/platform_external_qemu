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
import java.io.FileNotFoundException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.nio.file.FileSystemNotFoundException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;

/**
 * Loads a native library by searching a series of locations.
 */
final class NativeLibLoader {
    private NativeLibLoader() {}

    private static String getSharedExt() {
        String os = System.getProperty("os.name");
        if (os.startsWith("Windows")) {
            return ".dll";

        } else if (os.startsWith("Mac")) {
            return ".dylib";
        }
        return ".so";
    }

    private static Path getJarPath() {
        try {
            String path = NativeLibLoader.class.getProtectionDomain()
                                  .getCodeSource()
                                  .getLocation()
                                  .getPath();
            return Paths.get(URLDecoder.decode(path, "UTF-8")).getParent();
        } catch (Exception uee) {
            System.err.println("Unable to extract path from jar location: " + uee);
        }
        return Paths.get("");
    }

    private static boolean isValidSdkRoot(Path root) {
        return Files.exists(root.resolve("platforms"))
                && Files.exists(root.resolve("platform-tools")) && Files.isReadable(root);
    }

    private static Path getAndroidSdkRoot() {
        for (String option : new String[] {"ANDROID_HOME", "ANDROID_SDK_ROOT"}) {
            Path root = Paths.get(System.getenv().getOrDefault(option, ""));
            if (isValidSdkRoot(root)) {
                return Paths.get(root.toAbsolutePath().toString(), "emulator", "lib64");
            }
        }

        return Paths.get("");
    }

    /**
     * Tries to load the shared library of the given name. The shared library is
     * resolved by searching:
     *
     * - The current location of the jar file
     * - The ${cwd}/lib64 directory
     * - The $ANDROID_HOME/emulator/lib64 directory
     * - The $ANDROID_SDK_ROOT/emulator/lib64 directory.
     */
    public static void load(String name) {
        String lib = name + getSharedExt();
        Path paths[] =
                new Path[] {getJarPath(), Paths.get(".").resolve("lib64"), getAndroidSdkRoot()};
        for (Path option : paths) {
            Path resolved = option.resolve(lib).normalize().toAbsolutePath();
            if (Files.exists(option.resolve(lib))) {
                System.load(resolved.toString());
                return;
            }
        }
        throw new FileSystemNotFoundException(
                "Cannot find " + lib + " in any of " + Arrays.toString(paths));
    }
}
