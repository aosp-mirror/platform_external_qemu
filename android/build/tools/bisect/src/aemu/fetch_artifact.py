# Copyright 2023 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from aemu.android_build_client import AndroidBuildClient
from pathlib import Path
import logging
import zipfile
import subprocess
import os
from tempfile import TemporaryDirectory


class ZipFileWithAttr(zipfile.ZipFile):
    """Python does not set the file attributes properly."""

    def _extract_member(self, member, targetpath, pwd):
        if not isinstance(member, zipfile.ZipInfo):
            member = self.getinfo(member)

        targetpath = super()._extract_member(member, targetpath, pwd)

        attr = member.external_attr >> 16
        if attr != 0:
            os.chmod(targetpath, attr)
        return targetpath


def fetch_artifact(
    go_ab_service: AndroidBuildClient,
    destination: str,
    build_target: str,
    artifact: str,
    bid: str,
):
    """Fetches an artifact from go/ab to the given destination

    Args:
        go_ab_service (AndroidBuildClient): Service used to fetch artifacts
        destination (str): Directory where to store the artifact, and unzip it from
        build_target (str): build target, for example: emulator-linux_x64
        artifact (str): The artifact, can contain 'bid' that for build id. For example: sdk-repo-linux-emulator-{bid}.zip
        bid (str): The build id (usually a number) to fetch

    Returns:
        Path: Location of the obtained artifact.
    """
    assert destination.is_dir(), "Destination is not a directory"
    artifact = artifact.format(bid=bid)
    location = destination / artifact

    if location.exists():
        logging.warning("%s already exists, not downloading again.", location)
    else:
        logging.debug("Fetching: %s/%s/%s -> %s", bid, build_target, artifact, location)
        go_ab_service.fetch_bits(location, bid, build_target, artifact=artifact)

    return location


def invoke_shell_with_artifact(
    go_ab_service: AndroidBuildClient,
    dest: Path,
    build_target: str,
    artifact,
    cmd: str,
    bid: str,
) -> bool:
    """Invokes the given shell command after fetching and unzipping the received artifact

    Args:
        go_ab_service (AndroidBuildClient): Service used to fetch artifacts
        destination (str): Directory where to store the artifact, and unzip it from
        build_target (str): build target, for example: emulator-linux_x64
        artifact (str): The artifact, can contain 'bid' that for build id. For example: sdk-repo-linux-emulator-{bid}.zip
        cmd (str): The command to execute. This will be passed to "sh -c"
        bid (str): The build id (usually a number) to fetch

    Returns:
        bool: True if this artificat is ok, False otherwise.
    """
    assert dest.is_dir(), "Destination is not a directory"
    location = fetch_artifact(go_ab_service, dest, build_target, artifact, bid)
    env = os.environ.copy()
    env["X"] = bid
    env["ARTIFACT_LOCATION"] = str(location.absolute())

    # Unzip the artifact to a temporary location, if it is a zip file.
    if zipfile.is_zipfile(location):
        with TemporaryDirectory() as tmp_dir:
            with ZipFileWithAttr(location) as zip_file:
                zip_file.extractall(tmp_dir)
                env["ARTIFACT_UNZIP_DIR"] = str(tmp_dir)
                return subprocess.run(["sh", "-c", cmd], env=env).returncode == 0

    return subprocess.run(["sh", "-c", cmd], env=env).returncode == 0
