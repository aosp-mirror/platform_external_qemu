# -*- coding: utf-8 -*-
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
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List


def check_output(cmd, cwd: Path = None) -> str:
    """Runs the specified command and retrieves the command's output.

    This method logs the command before execution and captures the standard output.
    It ensures that the command runs successfully, suppressing standard error output.

    Args:
        cmd (List[str]): A list of strings representing the command to run.
        cwd (Path, optional): The working directory to run the command in.

    Returns:
        str: The captured standard output of the command as a string.

    Raises:
        subprocess.CalledProcessError: If the command exits with a non-zero status.
    """
    # Convert command elements to strings and log the command for visibility.
    cmd = [str(x) for x in cmd]
    logging.info("Run: %s", " ".join(cmd))
    return subprocess.check_output(
        cmd, encoding="utf-8", cwd=cwd, stderr=subprocess.DEVNULL, text=True
    )


def run(
    cmd: List[str],
    env: Dict[str, str] = None,
    cwd: Path = None,
    toolchain_path: Path = None,
) -> [str]:
    """Runs the given command, adding the 'toolchain' to the path

    Args:
        cmd: A list of strings representing the command to run.
        env: A dictionary of environment variables to set.
        cwd: The working directory to run the command in.

    Returns:
        The lines from stdout.
    """

    proc = Path(cmd[0])
    exe = shutil.which(proc.name, path=proc.parent) or shutil.which(proc.name)
    if not exe:
        raise FileNotFoundError(
            f"Executable {proc} could not be found on PATH: {os.environ.get('PATH', '')} or {proc.parent}"
        )

    cmd = [str(exe)] + [str(x) for x in cmd[1:]]
    logging.info("Run: [%s] %s", cwd, " ".join(cmd))

    if not env:
        env = os.environ

    # Let's make sure meson doesn't buffer aggressively
    env["PYTHONUNBUFFERED"] = "1"
    if cwd and not toolchain_path:
        toolchain_path = cwd / "toolchain"

    if toolchain_path:
        logging.info("Adding toolchain %s to path", toolchain_path)
        env["PATH"] = str(toolchain_path) + os.pathsep + env["PATH"]

    # Create a subprocess to run the command.
    process = subprocess.Popen(
        cmd,
        env=env,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1,
    )

    # Log the output of the command line by line.
    output = []
    for line in process.stdout:
        output.append(line.strip())
        logging.info(line.strip())

    # Wait for the command to finish and get the exit code.
    return_code = process.wait()

    # Log the final output and return code.
    logging.info("Command completed with exit code: %s", return_code)

    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, cmd)

    return output
