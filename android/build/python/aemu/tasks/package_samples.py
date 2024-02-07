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
import platform
import shutil
import zipfile
from pathlib import Path

from aemu.process.command import CommandFailedException
from aemu.process.py_runner import PyRunner
from aemu.tasks.build_task import BuildTask


class PackageSamplesTask(BuildTask):
    """Package the python gRPC samples."""

    def __init__(
        self,
        aosp: Path,
        build_directory: Path,
        distribution_directory: Path,
        target: str,
        sdk_build_number: str,
    ):
        super().__init__()
        self.build_directory = Path(build_directory)
        self.distribution_directory = (
            Path(distribution_directory) if distribution_directory else None
        )
        self.target = target
        self.sdk_build_number = sdk_build_number
        self.aosp = Path(aosp)

    def do_run(self):
        if self.target == "linux_aarch64" or self.target == "darwin-x86_64":
            logging.warning("Missing wheels for %s to create python distribution.", self.target)
            return

        sample_root = (
            self.aosp / "external" / "qemu" / "android" / "android-grpc" / "python"
        )
        aemu_grpc = sample_root / "aemu-grpc"
        snaptool = sample_root / "snaptool"
        samples = sample_root / "samples"

        # Note: we omit the file:// due to windows issues. This can result in
        # pip warnings.
        repo = self.aosp / "external" / "adt-infra" / "devpi" / "repo" / "simple"
        if platform.system() != "Windows":
            repo = f"file://{repo}"

        wheel_dir = self.build_directory / "wheels"
        wheel_dir.mkdir(parents=True, exist_ok=True)

        sample_dest = wheel_dir / "samples"
        shutil.rmtree(sample_dest, ignore_errors=True)
        shutil.copytree(samples, sample_dest)

        py = PyRunner(repo, self.aosp)
        py.run(
            [
                "-m",
                "pip",
                "wheel",
                "-i",
                repo,
                "-w",
                wheel_dir,
                aemu_grpc,
                snaptool,
                samples,
            ]
        )

        if not self.distribution_directory:
            logging.warning("No distribution directory.. skipping distribution")
            return

        self.distribution_directory.mkdir(exist_ok=True, parents=True)
        zip_name = (
            self.distribution_directory
            / f"sdk-repo-{self.target}-python-samples-{self.sdk_build_number}.zip"
        )

        logging.info("Creating zip: %s", zip_name)
        with zipfile.ZipFile(
            zip_name, "w", zipfile.ZIP_DEFLATED, allowZip64=True
        ) as zipf:
            for fname in wheel_dir.glob("**/*"):
                arcname = fname.relative_to(wheel_dir)
                logging.debug("Adding %s as %s", fname, arcname)
                zipf.write(fname, arcname)
