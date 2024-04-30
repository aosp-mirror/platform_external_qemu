## Generated Bazel Files

This directory contains platform-specific files extracted by the meson-to-bazel translator.

**Generating the Files**

To run the translator on the build bots:

1. Initiate a topic that includes the following change: [https://android-review.git.corp.google.com/c/platform/tools/buildSrc/+/3063204](https://android-review.git.corp.google.com/c/platform/tools/buildSrc/+/3063204)
2. Schedule builds for all relevant platforms (e.g., Darwin, Windows, Linux).

This will generate platform-specific `bazel-xxx.zip` files containing the Bazel sources.

**Incorporating the Files**

1. **Unzip:** Unzip each platform's `.zip` file into the appropriate directory. For example:

    ```bash
    unzip ~/Downloads/bazel-windows-amd64.zip -d $AOSP_ROOT/external/qemu
    ```

2. **Merge:** Combine the individual build files into a single BUILD.bazel file:

    ```bash
    python google/scripts/merge_bazel.py \
        --buildfile platform/BUILD.windows-amd64 @platforms//os:windows \
        --buildfile platform/BUILD.darwin-arm64 @platforms//os:macos \
        --buildfile platform/BUILD.linux-x86_64 @platforms//os:linux \
        --buildfile platform/BUILD.common common > BUILD.bazel
    ```

3. **Format:** Apply `buildifier` to clean up the generated file:

    ```bash
    buildifier BUILD.bazel
    ```

4. **Submit:** Create a change and submit it for review.
