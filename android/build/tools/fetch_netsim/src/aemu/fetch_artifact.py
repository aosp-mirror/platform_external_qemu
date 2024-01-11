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

import logging
import os
import subprocess
import zipfile
from pathlib import Path
from tempfile import TemporaryDirectory

import googleapiclient
from aemu.android_build_client import AndroidBuildClient
from tqdm import tqdm


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
    location = destination / artifact

    if not location.parent.exists():
        logging.info("Creating directory: %s", location.parent)
        location.parent.mkdir(parents=True)

    logging.info("Fetching: %s/%s/%s -> %s", bid, build_target, artifact, location)
    try:
        go_ab_service.fetch_bits(location, bid, build_target, artifact=artifact)
    except googleapiclient.errors.HttpError as e:
        logging.fatal(
            "Unable to retrieve %s (%s) from http://go/ab/%s",
            artifact,
            build_target,
            bid,
        )

    return location


def unzip_artifact(
    go_ab_service: AndroidBuildClient,
    dest: Path,
    build_target: str,
    artifact,
    bid: int,
):
    """Invokes the given shell command after fetching and unzipping the received artifact

    Args:
        go_ab_service (AndroidBuildClient): Service used to fetch artifacts
        dest (Path): Directory where to store the artifact, and unzip it from
        build_target (str): build target, for example: emulator-linux_x64
        artifact (str): The artifact, can contain 'bid' that for build id. For example: sdk-repo-linux-emulator-{bid}.zip
        bid (int): The build id (usually a number) to fetch
    """
    assert dest.is_dir(), f"Destination {dest} is not a directory"
    with TemporaryDirectory(prefix="netsim") as tmp:
        location = fetch_artifact(
            go_ab_service, Path(tmp), build_target, artifact, bid
        ).absolute()

        if not zipfile.is_zipfile(location):
            raise Exception(f"The artifact {location} is not a valid zipfile")

        logging.info("Unzipping %s -> %s", location, dest)
        with ZipFileWithAttr(location) as zip_ref:
            # Get a list of all the files in the zip file
            file_list = zip_ref.namelist()

            # Create a progress bar using tqdm
            with tqdm(total=len(file_list), unit="file") as progress_bar:
                # Extract each file from the zip archive
                for file_name in file_list:
                    zip_ref.extract(file_name, dest)
                    progress_bar.update(1)
