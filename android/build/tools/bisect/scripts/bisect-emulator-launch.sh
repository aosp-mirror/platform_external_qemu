#!/bin/sh

# This script demonstrates how to use `emu-bisect` to find a UI issue that occurs during emulator launch.

# **Overview**
# `emu-bisect` uses binary search to identify a specific build where a shell command starts to fail.
# In this case, we're looking for a build where the emulator fails to launch correctly, exhibiting a UI issue.

# **Prerequisites**
# Ensure you have a valid OAuth token if running on Mac OS or Windows.

# **Usage**
# The following command runs `emu-bisect` with the specified options:

emu-bisect \
    --num 1024 \  # Search the last 1024 builds
    --artifact sdk-repo-linux-emulator-{bid}.zip \  # Specify the artifact to download
    --build_target emulator-linux_x64_gfxstream \  # Specify the build target
    --unzip \  # Unzip the downloaded artifact
    '${ARTIFACT_UNZIP_DIR}/emulator/emulator @V -wipe-data; read -p "Is $X OK? (y/n): " ok < /dev/tty;  [[ "$ok" == "y" ]]'

# **Explanation**
# * `--num 1024`:  Limits the search to a maximum of 1024 builds. Since 2^10 = 1024, the script will perform 9-10 iterations.
# * `--artifact sdk-repo-linux-emulator-{bid}.zip`:  Specifies the artifact to download, where `{bid}` is replaced with the build ID.
# * `--build_target emulator-linux_x64_gfxstream`: Specifies the build target for the emulator.
# * `--unzip`:  Automatically unzips the downloaded artifact.
# * `'${ARTIFACT_UNZIP_DIR}/emulator/emulator @V -wipe-data; read -p "Is $X OK? (y/n): " ok < /dev/tty;  [[ "$ok" == "y" ]]'`: This is the shell command that is executed for each build.
#    * `${ARTIFACT_UNZIP_DIR}/emulator/emulator`: Launches the emulator from the unzipped artifact.
#    * `@V`:  Specifies the AVD (Android Virtual Device) to use.
#    * `-wipe-data`:  Wipes the data of the AVD before launching.
#    * `read -p "Is $X OK? (y/n): " ok < /dev/tty`:  Prompts the user to indicate whether the emulator launched correctly (y/n).
#    * `[[ "$ok" == "y" ]]`:  Checks the user's input. The command returns 0 (success) if the user enters "y", and 1 (failure) otherwise.

